/*======================================================================

  This file is part of the elastix software.

  Copyright (c) University Medical Center Utrecht. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/

#ifndef __itkAffineLogTransform_txx
#define __itkAffineLogTransform_txx


#include "vnl/vnl_matrix_exp.h"
#include "itkAffineLogTransform.h"

namespace itk
{

// Constructor with default arguments
template <class TScalarType, unsigned int Dimension>
AffineLogTransform<TScalarType, Dimension>
::AffineLogTransform():
  Superclass(ParametersDimension)
{
  this->m_MatrixLogDomain.SetIdentity();
  this->m_MatrixNormalDomain.SetIdentity();
  this->m_Matrix.SetIdentity();
  this->m_Offset.Fill( itk::NumericTraits<ScalarType>::Zero );
  this->PrecomputeJacobianOfSpatialJacobian();
}

// Constructor with default arguments
template <class TScalarType, unsigned int Dimension>
AffineLogTransform<TScalarType, Dimension>
::AffineLogTransform(const MatrixType & matrix,
                   const OutputPointType & offset)
{
  this->SetMatrix(matrix);

  OffsetType off;
  for(unsigned int i = 0; i < Dimension; i ++)
  {
	off[i] = offset[i];
  }
  this->SetOffset(off);
  // this->ComputeMatrix?
  this->PrecomputeJacobianOfSpatialJacobian();
}

// Set Parameters
template <class TScalarType, unsigned int Dimension>
void
AffineLogTransform<TScalarType, Dimension>
::SetParameters( const ParametersType & parameters )
{
   itkDebugMacro( << "Setting parameters " << parameters );
  unsigned int d = Dimension;
  this->ComputeMatrixNormalDomain();
  unsigned int blockoffset = d*d;
  for( unsigned int i = 0; i < d; i++)
  {
      this->m_Offset[i] = parameters[i+blockoffset];
  }
  std::cout << "parameters: " << parameters << std::endl;

  this->SetMatrix( this->m_MatrixNormalDomain );
  this->SetOffset( this->m_Offset );
  
  // Modified is always called since we just have a pointer to the
  // parameters and cannot know if the parameters have changed.

  this->Modified();
  itkDebugMacro(<<"After setting parameters ");
}

// Compute the log domain matrix
template <class TScalarType, unsigned int Dimension>
void
AffineLogTransform<TScalarType, Dimension>
::ComputeMatrixLogDomain( void )
{
    unsigned int d = Dimension;
    unsigned int j = 0;
    MatrixType matrix;

    for(unsigned int k = 0; k < d; k++)
    {
        for(unsigned int l = 0; l < d; l++)
        {
            matrix(k,l) = this->m_Matrix(k,l);
        }
    }

    this->m_MatrixLogDomain = matrix;
}

// Compute the log domain matrix
template <class TScalarType, unsigned int Dimension>
void
AffineLogTransform<TScalarType, Dimension>
::ComputeMatrixNormalDomain( void )
{
    unsigned int d = Dimension;
    unsigned int j = 0;
    MatrixType exponentMatrix;

    this->ComputeMatrixLogDomain();
    exponentMatrix = vnl_matrix_exp( this->m_MatrixLogDomain.GetVnlMatrix() );

    for( unsigned int i = 0; i < d; i++)
    {
        for(unsigned int j = 0; j < d; j++)
        {
          this->m_MatrixNormalDomain(i,j) = exponentMatrix(i,j);
        }
    }

    this->m_MatrixNormalDomain = exponentMatrix;
}

// Get Parameters
template <class TScalarType, unsigned int Dimension>
const typename AffineLogTransform<TScalarType, Dimension>::ParametersType &
AffineLogTransform<TScalarType, Dimension>
::GetParameters( void ) const
{
    unsigned int k = 0;
    for(unsigned int i = 0; i < Dimension; i++)
    {
        for(unsigned int j = 0; j < Dimension; j++)
        {
            this->m_Parameters[k] = this->m_MatrixNormalDomain(i,j);
            k += 1;
        }
    }

    for(unsigned int j = 0; j < Dimension; j++)
    {
        this->m_Parameters[k] = this->GetTranslation()[j];
        k += 1;
    }
    std::cout << "parameters: " << this->m_Parameters << std::endl;
    return this->m_Parameters;
}

// SetIdentity
template <class TScalarType, unsigned int Dimension>
void
AffineLogTransform<TScalarType, Dimension>
::SetIdentity( void )
{
  Superclass::SetIdentity();
  this->m_MatrixNormalDomain.SetIdentity();
  this->PrecomputeJacobianOfSpatialJacobian();
}

//Get Jacobian
template<class TScalarType, unsigned int Dimension>
void
AffineLogTransform<TScalarType, Dimension>::
GetJacobian( const InputPointType & p,
    JacobianType & j,
    NonZeroJacobianIndicesType & nzji) const
{
    unsigned int d = Dimension;
    unsigned int ParametersDimension = d*(d+1);
  j.SetSize(d, ParametersDimension );
  j.Fill(itk::NumericTraits<ScalarType>::Zero);

  const JacobianOfSpatialJacobianType & jsj = this->m_JacobianOfSpatialJacobian;

  const InputVectorType pp = p - this->GetCenter();
  for(unsigned int dim=0; dim < d*d; dim++ )
  {
    const InputVectorType column = jsj[dim] * pp;
    for (unsigned int i=0; i < d; ++i)
    {
      j(i,dim) = column[i];
    }
  }

  // compute derivatives for the translation part
  const unsigned int blockOffset = d*d;
  for(unsigned int dim=0; dim < Dimension; dim++ )
  {
    j[ dim ][ blockOffset + dim ] = 1.0;
  }
  std::cout << "point: " << pp << "\njacobian: " << j << std::endl;

}

// Precompute Jacobian of Spatial Jacobian
template <class TScalarType, unsigned int Dimension>
void
AffineLogTransform<TScalarType, Dimension>
::PrecomputeJacobianOfSpatialJacobian( void )
{
    this->ComputeMatrixLogDomain();
    unsigned int d = Dimension;
    unsigned int ParametersDimension = d*(d+1);
    
    /** The Jacobian of spatial Jacobian is constant over inputspace, so is precomputed */
    JacobianOfSpatialJacobianType & jsj = this->m_JacobianOfSpatialJacobian;

    jsj.resize(ParametersDimension);

    vnl_matrix< ScalarType > dA(d,d);
    vnl_matrix< ScalarType > dummymatrix(d,d);
    vnl_matrix< ScalarType > A_bar(2*d,2*d);
    vnl_matrix< ScalarType > B_bar(2*d,2*d);

    dA.fill(itk::NumericTraits<ScalarType>::Zero);
    dummymatrix.fill(itk::NumericTraits<ScalarType>::Zero);
    A_bar.fill(itk::NumericTraits<ScalarType>::Zero);

    // Fill A_bar top left and bottom right with A
    for(unsigned int k = 0; k < d; k++)
    {
        for(unsigned int l = 0; l < d; l++)
        {
            A_bar(k,l) = this->m_MatrixLogDomain(k,l);
        }
    }
    for(unsigned int k = d; k < 2*d; k++)
    {
        for(unsigned int l = d; l < 2*d; l++)
        {
            A_bar(k,l) = this->m_MatrixLogDomain(k-d,l-d);
        }
    }

    unsigned int m = 0; //Dummy loop index

    //Non-translation derivatives
    for(unsigned int i = 0; i < d; i++)
    {
        for(unsigned int j = 0; j < d; j++)
        {
            dA(i,j) = 1;
            for(unsigned int k = 0; k < d; k++)
            {
                for(unsigned int l = d; l < 2*d; l++)
                {
                    A_bar(k,l) = dA(k,(l-d));
                }
            }
            B_bar = vnl_matrix_exp( A_bar );
            for(unsigned int k = 0; k < d; k++)
            {
                for(unsigned int l = d; l < 2*d; l++)
                {
                    dummymatrix(k,(l-d)) = B_bar(k,l);
                }
            }
			jsj[m] = dummymatrix;
            dA.fill(itk::NumericTraits<ScalarType>::Zero);
            m += 1;
        }
    }
  /** Translation parameters: */
  for ( unsigned int par = d*d; par < d+(d*d); ++par )
  {
    jsj[par].Fill(itk::NumericTraits<ScalarType>::Zero);
  } 

  for(unsigned int i = 0; i < jsj.size(); i++)
  {
      std::cout << jsj[i] << std::endl;
  }
}


// Print self
template<class TScalarType, unsigned int Dimension>
void
AffineLogTransform<TScalarType, Dimension>::
PrintSelf(std::ostream &os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);

  os << indent << "parameters:" << this->m_Parameters << std::endl;

}

} // end namespace

#endif // itkAffineLogTransform_TXX
