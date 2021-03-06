/*
 *  Copyright © 2009-2014 The Regents of the University of California.
 *  All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *  •   Redistributions of source code must retain the above copyright notice, 
 *      this list of conditions and the following disclaimer.
 *  •   Redistributions in binary form must reproduce the above copyright notice, 
 *      this list of conditions and the following disclaimer in the documentation 
 *      and/or other materials provided with the distribution.
 *  •   None of the names of any campus of the University of California, the name 
 *      "The Regents of the University of California," or the names of any of its 
 *      contributors may be used to endorse or promote products derived from this 
 *      software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 *  OF SUCH DAMAGE.
 */



/*
 *  $URL$
 *  $Rev$
 *  $Author$
 *  $Date$
 *
 *  Authors:
 *      Jason C. Crane, Ph.D.
 *      Beck Olson,
 */


#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkImageFourierFilter.h> // for vtkImageComplex struct
#include <vtkMath.h>

#include <svkMRSFirstPointPhase.h>
#include <svkMRSFirstPointPhaseCostFunction.h>
#include <svkPhaseSpec.h>
#include <svkMrsImageFFT.h>


using namespace svk;


vtkCxxRevisionMacro(svkMRSFirstPointPhase, "$Rev$");
vtkStandardNewMacro(svkMRSFirstPointPhase);


/*!
 *
 */
svkMRSFirstPointPhase::svkMRSFirstPointPhase()
{
#if VTK_DEBUG_ON
    this->DebugOn();
#endif

    vtkDebugMacro(<< this->GetClassName() << "::" << this->GetClassName() << "()");
    this->seriesDescription = "FirstPointPhaseMap"; 
    
}

/*!
 *
 */
svkMRSFirstPointPhase::~svkMRSFirstPointPhase()
{
}


/*!
 *  Make sure data is in time domain initiall for first point phasing
 */
void svkMRSFirstPointPhase::PrePhaseSetup()
{

    svkMrsImageData* data = svkMrsImageData::SafeDownCast(this->GetImageDataInput(0));

    //  First point phase is in time domain: 

    //  Get the domain of the input spectra:  time/frequency
    string spectralDomain = data->GetDcmHeader()->GetStringValue( "SignalDomainColumns");
    if ( spectralDomain.compare("TIME") == 0 ) {
        this->isSpectralFFTRequired = false; 
        cout << "TIME DOMAIN INPUT" << endl;
    } else {
        this->isSpectralFFTRequired = true;
        cout << "FREQ DOMAIN INPUT" << endl;
    }

    //  if necessary, transform data to time domain: 
    if ( this->isSpectralFFTRequired == true ) {

        svkMrsImageFFT* fft = svkMrsImageFFT::New();
        fft->SetInput( data );

        fft->SetFFTDomain( svkMrsImageFFT::SPECTRAL );

        //  frequency to time: 
        fft->SetFFTMode( svkMrsImageFFT::REVERSE );

        fft->Update();
        fft->Delete();

    }

}


/*!
 *
 */
void svkMRSFirstPointPhase::PostPhaseCleanup()
{

    //  First order phase is in time domain: 

    //  if necessary, transform data to time domain: 
    if ( this->isSpectralFFTRequired == true ) {

        svkMrsImageData* data = svkMrsImageData::SafeDownCast(this->GetImageDataInput(0));

        svkMrsImageFFT* fft = svkMrsImageFFT::New();
        fft->SetInput( data );

        fft->SetFFTDomain( svkMrsImageFFT::SPECTRAL );

        //  frequency to time: 
        fft->SetFFTMode( svkMrsImageFFT::FORWARD );

        fft->Update();
        fft->Delete();

    }

}


/*!
 * 
 */
#ifdef SWARM
void svkMRSFirstPointPhase::InitOptimizer( int cellID, itk::ParticleSwarmOptimizer::Pointer itkOptimizer )
#else
void svkMRSFirstPointPhase::InitOptimizer( int cellID, itk::PowellOptimizer::Pointer itkOptimizer )
#endif

{

    svkMrsImageData* data = svkMrsImageData::SafeDownCast(this->GetImageDataInput(0));
    vtkFloatArray* spectrum = static_cast<vtkFloatArray*>( data->GetSpectrum( cellID ) );
    float cmplxPt[2];
    spectrum->GetTupleValue(0, cmplxPt);
    //cout << " cell: " << cellID << " -> " << cmplxPt[0] << endl;

    //========================================================
    //  ITK Optimization 
    //========================================================
    svkMRSFirstPointPhaseCostFunction::Pointer costFunction = svkMRSFirstPointPhaseCostFunction::New();
    itkOptimizer->SetCostFunction( costFunction.GetPointer() );

    costFunction->SetSpectrum( spectrum ); 
    costFunction->SetNumFreqPoints(this->numTimePoints ); 
    //costFunction->SetPeakPicker( this->peakPicker ); 
    //costFunction->SetLinearPhaseArrays( this->linearPhaseArrays ); 
    //costFunction->SetNumFirstOrderPhaseValues( this->numFirstOrderPhaseValues ); 

    //  set dimensionality of parameter space: 
    const unsigned int spaceDimension = costFunction->GetNumberOfParameters();
    typedef svkPhaseCostFunction::ParametersType ParametersType;
    ParametersType  initialPosition( spaceDimension );
    initialPosition[0] =  0.;

#ifdef SWARM 

        //  Set bounds
        itk::ParticleSwarmOptimizer::ParameterBoundsType bounds;
        // Zero order phase range radians
        bounds.push_back( std::make_pair( -1 * vtkMath::Pi(), vtkMath::Pi()) ); 
        itkOptimizer->SetParameterBounds( bounds );

        unsigned int numberOfParticles = 10;
        unsigned int maxIterations = 500;
        double xTolerance = 0.01;
        double fTolerance = 0.01;
        itk::ParticleSwarmOptimizer::ParametersType 
            initialParameters( spaceDimension ), finalParameters;
        itkOptimizer->SetNumberOfParticles( numberOfParticles );
        itkOptimizer->SetMaximalNumberOfIterations( maxIterations );
        itkOptimizer->SetParametersConvergenceTolerance( xTolerance,
                                                   costFunction->GetNumberOfParameters() );
        itkOptimizer->SetFunctionConvergenceTolerance( fTolerance );
        itkOptimizer->SetInitializeNormalDistribution( false ); // use uniform distribution

        //  required to get reproducible results, otherwise uses random seed
        itkOptimizer->SetUseSeed( true ); // use uniform distribution
        itkOptimizer->SetSeed( 7 ); // use uniform distribution


#else
        itkOptimizer->SetStepLength( 1 );
        itkOptimizer->SetStepTolerance( 1);
        itkOptimizer->SetValueTolerance( 1 );
        itkOptimizer->SetMaximumIteration( 10000 );
#endif 

    itkOptimizer->SetInitialPosition( initialPosition );

}


/*!
 * 
 */
void svkMRSFirstPointPhase::FitPhase( int cellID )  
{

#ifdef SWARM 
        typedef itk::ParticleSwarmOptimizer OptimizerType;
#else
        typedef itk::PowellOptimizer        OptimizerType;
#endif

    OptimizerType::Pointer itkOptimizer = OptimizerType::New();
    this->InitOptimizer( cellID, itkOptimizer ); 

#ifndef SWARM 
    //  maximize total peak height
    itkOptimizer->SetMaximize( true );
#endif

    try { 
        //cout << "CELL ID Optimize: " << cellID << endl;
        itkOptimizer->StartOptimization();
    } catch( itk::ExceptionObject & e ) {
        cout << "Exception thrown ! " << endl;
        cout << "An error ocurred during Optimization" << endl;
        cout << "Location    = " << e.GetLocation()    << endl;
        cout << "Description = " << e.GetDescription() << endl;
        //return EXIT_FAILURE;
        return; 
    }

    typedef svkPhaseCostFunction::ParametersType ParametersType;
    ParametersType finalPosition = itkOptimizer->GetCurrentPosition();

    //
    // check results to see if it is within range
    //
    bool pass = true;

    // Exercise various member functions.
    cout << endl;

    //itkOptimizer->Print( cout );

    if( !pass ) {
        cout << "Test failed." << endl;
        return;
    } 
    //cout << "Test passed." << endl;

    //  ===================================================
    //  Apply fitted phase values to spectrum: 
    //  ===================================================
    svkMrsImageData* data = svkMrsImageData::SafeDownCast(this->GetImageDataInput(0));
    vtkFloatArray* spectrum = static_cast<vtkFloatArray*>( data->GetSpectrum( cellID ) );

    float phi0Final = finalPosition[0]; 
    cout << "           PHI0 FINAL("<<cellID <<"): " << phi0Final * 180. / vtkMath::Pi() << endl;
    svkPhaseSpec::ZeroOrderPhase( phi0Final, spectrum); 


    //  ===================================================
    //  Write the fitted phase values to a parameter map: 
    //  ===================================================
    svkMriImageData* mriData = svkMriImageData::SafeDownCast(this->GetOutput(0));

    //  map the mrsCellID to the correct output map volume:     
    svkDcmHeader::DimensionVector mriDimensionVector = mriData->GetDcmHeader()->GetDimensionIndexVector();
    int numSpatialVoxels = svkDcmHeader::GetNumSpatialVoxels(&mriDimensionVector);
    int volumeNumber = static_cast<int>(cellID/numSpatialVoxels); 
    this->mapArrayZeroOrderPhase = mriData->GetPointData()->GetArray(volumeNumber);
    float phi0FinalDegrees = phi0Final * 180. / vtkMath::Pi(); 

    //  put this in the correct spatial location: 
    svkDcmHeader::DimensionVector loopVector = mriDimensionVector; 
    svkDcmHeader::GetDimensionVectorIndexFromCellID( &mriDimensionVector, &loopVector, cellID); 
    int spatialCellID = mriData->GetDcmHeader()->GetSpatialCellIDFromDimensionVectorIndex( &mriDimensionVector, &loopVector); 
    this->mapArrayZeroOrderPhase->SetTuple1(spatialCellID, phi0FinalDegrees);

    return;
}


void svkMRSFirstPointPhase::ValidateInput()
{
}

