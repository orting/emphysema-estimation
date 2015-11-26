#ifndef __ClusterModel_h
#define __ClusterModel_h

/*
  A cluster model should label instances according to 
  a basis given by labelled centers and a distance function

 */

#include "flann/flann.hpp"

#include "Types.h"

template< typename TDistanceFunctor >
class ClusterModel {
public:
  typedef ClusterModel< TDistanceFunctor > Self;
  
  typedef ee_llp::DoubleRowMajorMatrixType MatrixType;
  typedef ee_llp::DoubleColumnVectorType VectorType;
  typedef std::vector< int > IndexVectorType;
  typedef double ElementType;
  typedef TDistanceFunctor DistanceFunctorType;

  ClusterModel( )
    : m_Centers(  ),
      m_Labels(  ),
      m_Weights(  ),
      m_Index( ),
      m_InternalCenters( )
  {}

  ClusterModel( Self&& other )
    : m_Centers( std::move(other.m_Centers) ),
      m_Labels( std::move(other.m_Labels) ),
      m_Weights( std::move(other.m_Weights) ),
      m_Index( std::move(other.m_Index) ),
      m_InternalCenters( std::move(other.m_InternalCenters) )
  {}
  
  ClusterModel( const MatrixType& centers,
		const VectorType& labels,
		const VectorType& weights )
    : m_Centers( centers ),
      m_Labels( labels ),
      m_Weights( weights ),
      m_Index( ),
      m_InternalCenters( )
  {}

  ~ClusterModel() {}


  void setCenters( const MatrixType& centers ) {
    m_Centers = centers;
  }

  void setLabels( const VectorType& labels ) {
    m_Labels = labels;
  }

  VectorType& weights( ) {  return m_Weights;  }
  const VectorType& weights() const { return m_Weights;  }
  
  void build() {
    // This is so ugly
    m_InternalCenters =
      std::move(
		std::unique_ptr<InternalMatrixType>(
      new InternalMatrixType( const_cast<ElementType*>(m_Centers.data()),
			      m_Centers.rows(),
			      m_Centers.cols()
			      )
						    ));
    
    DistanceFunctorType dist( m_Weights.data(),
			      m_Weights.size() );
    
    m_Index = std::move( std::unique_ptr< IndexType > (new IndexType( *m_InternalCenters,
			     IndexParamsType(),
								      dist )
						       ));
    m_Index->buildIndex( );
  }

  
  VectorType
  predictInstances( const MatrixType& instances ) const {
    // We want to find exact matches
    SearchParamsType searchParams( flann::FLANN_CHECKS_UNLIMITED );
    searchParams.use_heap = flann::FLANN_False;
    searchParams.cores = 0; // Use as many as you like
    
    // Allocate matrices for the query
    IndexVectorType indices( instances.rows() );
    InternalIndexVectorType _indices( indices.data(), indices.size(), 1 );
    MatrixType distances( instances.rows(), 1 );
    InternalMatrixType _distances( distances.data(), distances.rows(), distances.cols() );

    InternalMatrixType
      _instances( const_cast<ElementType*>(instances.data()),
		  instances.rows(),
		  instances.cols() );

    m_Index->knnSearch( _instances, _indices, _distances, 1, searchParams );
    VectorType predictions( indices.size() );
    for ( std::size_t i = 0; i < predictions.size(); ++i ) {
      predictions(i) = m_Labels( indices[i] );
    }
    return std::move( predictions );
  }

  VectorType
  predictBags( const MatrixType& instances,
	       const IndexVectorType& bagLabels,
	       const std::size_t numberOfBags ) const {
    VectorType instancePrediction = predictInstances( instances );
    VectorType bagPrediction = VectorType::Zero( numberOfBags );
    VectorType bagCounts = VectorType::Zero( numberOfBags );
    for ( std::size_t i = 0; i < bagLabels.size(); ++i ) {
      bagPrediction( bagLabels[i] ) += instancePrediction(i);
      bagCounts( bagLabels[i] ) += 1; 
    }
    return std::move( bagPrediction.cwiseQuotient( bagCounts ) );
  }

private:
  typedef typename flann::LinearIndexParams IndexParamsType;
  typedef typename flann::SearchParams SearchParamsType;
  typedef typename flann::LinearIndex< DistanceFunctorType > IndexType;
  typedef flann::Matrix<ElementType> InternalMatrixType;
  typedef flann::Matrix<int> InternalIndexVectorType;
  
  MatrixType m_Centers;
  VectorType m_Labels;
  VectorType m_Weights;
  std::unique_ptr<IndexType> m_Index;
  std::unique_ptr<InternalMatrixType> m_InternalCenters;    
};

#endif
