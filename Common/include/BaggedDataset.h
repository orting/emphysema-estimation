#ifndef __BaggedDataset_h
#define __BaggedDataset_h

#include <stdexcept>
#include <type_traits>
#include "Eigen/Dense"

template< size_t BagLabelDim=1, size_t InstanceLabelDim=1 >
class BaggedDataset {
public:
  typedef BaggedDataset< BagLabelDim, InstanceLabelDim > Self;
  // In some cases we MUST have that the matrix is stored in row-major order
  // unfortunately we cannot make row-major matrix with a single column unless
  // we make it Dynamic.
  // For this reason we use row-major for the matrix and col-major for the label
  // vectors when they have dim=1
  typedef Eigen::Matrix< double,
			 Eigen::Dynamic,
			 Eigen::Dynamic,
			 Eigen::RowMajor > MatrixType;

  typedef Eigen::Matrix< double,
   			 Eigen::Dynamic,
			 InstanceLabelDim,
			 InstanceLabelDim == 1 ? Eigen::ColMajor : Eigen::RowMajor > InstanceLabelVectorType;

  typedef Eigen::Matrix< double,
  			 Eigen::Dynamic,
  			 BagLabelDim,
  			 BagLabelDim == 1 ? Eigen::ColMajor : Eigen::RowMajor > BagLabelVectorType;

  typedef Eigen::Matrix< size_t,
  			 Eigen::Dynamic,
  			 1,
  			 Eigen::ColMajor > IndexVectorType;


  BaggedDataset()
    : m_Instances( )
    , m_BagMembershipIndices(  )
    , m_BagLabels(  )
    , m_InstanceLabels(  )
  {}   
  
  BaggedDataset( const MatrixType& instances,
		 const IndexVectorType& bagMembershipIndices,
		 const BagLabelVectorType& bagLabels,
		 const InstanceLabelVectorType& instanceLabels )		 
    : m_Instances( instances)
    , m_BagMembershipIndices( bagMembershipIndices )
    , m_BagLabels( bagLabels )
    , m_InstanceLabels( instanceLabels )
  {
    if ( instances.rows() != instanceLabels.rows() ) {
      throw std::logic_error( "Number of instance labels do not match number of instances" );
    }
    if ( instances.rows() != bagMembershipIndices.rows() ) {
      throw std::logic_error( "Number of bag membership indices do not match number of instances" );
    }
    if ( bagMembershipIndices.maxCoeff() >= bagLabels.rows() ) {
      throw std::logic_error( "Largest bag membership index is larger than the number of bag labels" );
    }    
  }


  BaggedDataset( const BaggedDataset& o )
    : Self( o.m_Instances, o.m_BagMembershipIndices, o.m_BagLabels, o.m_InstanceLabels )
  {}    
  
  size_t NumberOfBags() const {
    return m_BagLabels.rows();
  }
  
  size_t NumberOfInstances() const {
    return m_Instances.rows();
  }

  size_t Dimension() const {
    return m_Instances.cols();
  }

  const IndexVectorType& Indices() const {
    return m_BagMembershipIndices;
  }

  const MatrixType& Instances() const {
    return m_Instances;
  }

  const BagLabelVectorType& BagLabels() const {
    return m_BagLabels;
  }

  const InstanceLabelVectorType& InstanceLabels() const {
    return m_InstanceLabels;
  }

  void InstanceLabels(const InstanceLabelVectorType& instanceLabels) {
    if ( instanceLabels.rows() != m_Instances.rows() ) {
      throw std::logic_error( "Number of instance labels do not match number of instances" );
    }
    m_InstanceLabels = instanceLabels;
  }

  static
  Self
  Random( size_t numberOfBags, size_t bagSize, size_t dimension ) {
    size_t numberOfInstances = numberOfBags * bagSize;
    MatrixType instances = MatrixType::Random( numberOfInstances, dimension );
    IndexVectorType bagMembership = IndexVectorType( numberOfInstances );
    for ( size_t i = 0, index = 0; i < bagMembership.size(); ++i ) {
      if ( i >= (index + 1) * bagSize ) {
	++index;
      }
      bagMembership(i) = index;
    }
    BagLabelVectorType bagLabels = BagLabelVectorType::Random( numberOfBags, BagLabelDim );
    InstanceLabelVectorType instanceLabels = InstanceLabelVectorType::Zero( numberOfInstances, InstanceLabelDim );
    return Self( instances, bagMembership, bagLabels, instanceLabels );
  }  


  std::ostream& Save( std::ostream& os ) const {
    os << "#  number of instances"
       << "   number of features"
       << "   number of bags"
       << "   dimension of bag label space"
       << "   dimension of instances label space"
       << std::endl
       <<          m_Instances.rows()
       << "   " << m_Instances.cols()
       << "   " << m_BagLabels.rows()
       << "   " << m_BagLabels.cols()
       << "   " << m_InstanceLabels.cols()
       << std::endl;
    os.write( reinterpret_cast< const char* >( m_Instances.data() ), sizeof(double)*m_Instances.size() );
    os.write( reinterpret_cast< const char* >( m_BagMembershipIndices.data() ), sizeof(size_t)*m_BagMembershipIndices.size() );    
    os.write( reinterpret_cast< const char* >( m_BagLabels.data() ), sizeof(double)*m_BagLabels.size() );
    os.write( reinterpret_cast< const char* >( m_InstanceLabels.data() ), sizeof(double)*m_InstanceLabels.size() );
    return os;
  }


  static Self
  Load( std::istream& is ) {
    char c;
    is >> c;
    if ( c != '#' ) {
      is.setstate( std::ios::failbit );
      throw std::runtime_error( "Missing header" );
    }
    // Skip the line
    is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    std::size_t nInstances, nFeatures, nBags, bagLabelDim, instanceLabelDim;
    is >> nInstances >> nFeatures >> nBags >> bagLabelDim >> instanceLabelDim;
    if ( !is ) {
      is.setstate( std::ios::failbit );
      throw std::runtime_error( "Error parsing header" );
    }
    // We still have a newline we need to read
    is.get();
    
    MatrixType instances( nInstances, nFeatures );
    IndexVectorType bagMembershipIndices( nInstances );
    BagLabelVectorType bagLabels( nBags, bagLabelDim );
    InstanceLabelVectorType instanceLabels( nInstances, instanceLabelDim );

    double buf;
    char* bufPtr = reinterpret_cast< char* >( &buf );
    for ( std::size_t i = 0; i < nInstances; ++i ) {
      for ( std::size_t j = 0; j < nFeatures; ++j ) {
	if ( ! is.read( bufPtr, sizeof buf ) ) {
	  throw std::runtime_error( "Could not read instances" );
	}
	instances(i,j) = buf;      
      }
    }

    size_t size_t_Buf;
    char* size_t_BufPtr = reinterpret_cast< char* >( &size_t_Buf );
    for ( std::size_t i = 0; i < nInstances; ++i ) {
      if ( ! is.read( size_t_BufPtr, sizeof size_t_Buf ) ) {
	throw std::runtime_error( "Could not read bag membership indices" );
      }
      bagMembershipIndices(i) = size_t_Buf;
    }

    for ( std::size_t i = 0; i < nBags; ++i ) {
      for ( std::size_t j = 0; j < bagLabelDim; ++j ) {
	if ( ! is.read( bufPtr, sizeof buf ) ) {
	  throw std::runtime_error( "Could not read bag labels" );
	}
    	bagLabels(i,j) = buf;
      }
    }

    for ( std::size_t i = 0; i < nInstances; ++i ) {
      for ( std::size_t j = 0; j < instanceLabelDim; ++j ) {
	if ( ! is.read( bufPtr, sizeof buf ) ) {
	  throw std::runtime_error( "Could not read instances labels" );
	}
    	instanceLabels(i,j) = buf;
      }
    }

    return Self( instances, bagMembershipIndices, bagLabels, instanceLabels );
  }


  inline bool operator==(const Self& o) const {
    return
      ( m_Instances.rows() == o.m_Instances.rows() ) &&
      ( m_Instances.cols() == o.m_Instances.cols() ) &&
      ( m_BagMembershipIndices.rows() == o.m_BagMembershipIndices.rows() ) &&
      ( m_BagLabels.rows() == o.m_BagLabels.rows() ) &&
      ( m_BagLabels.cols() == o.m_BagLabels.cols() ) &&
      ( m_InstanceLabels.rows() == o.m_InstanceLabels.rows() ) &&
      ( m_InstanceLabels.cols() == o.m_InstanceLabels.cols() ) &&
      ( std::equal(m_Instances.data(), m_Instances.data() + m_Instances.size(), o.m_Instances.data() ) ) &&
      ( std::equal(m_BagMembershipIndices.data(), m_BagMembershipIndices.data() + m_BagMembershipIndices.size(), o.m_BagMembershipIndices.data() ) ) &&
      ( std::equal(m_BagLabels.data(), m_BagLabels.data() + m_BagLabels.size(), o.m_BagLabels.data() ) ) &&
      ( std::equal(m_InstanceLabels.data(), m_InstanceLabels.data() + m_InstanceLabels.size(), o.m_InstanceLabels.data() ) );
  }

  inline bool operator!=( const Self& o ) const {
    return !( *this == o );
  }

  
private:
  MatrixType m_Instances;
  IndexVectorType m_BagMembershipIndices;
  BagLabelVectorType m_BagLabels;
  InstanceLabelVectorType m_InstanceLabels;
};

#endif
