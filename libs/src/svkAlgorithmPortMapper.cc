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
 *      Beck Olson
 */



#include <svkAlgorithmPortMapper.h>


using namespace svk;

vtkCxxRevisionMacro(svkAlgorithmPortMapper, "$Rev$");
vtkStandardNewMacro(svkAlgorithmPortMapper);


/*!
 *
 */
svkAlgorithmPortMapper::svkAlgorithmPortMapper()
{
#if VTK_DEBUG_ON
    this->DebugOn();
#endif

    vtkDebugMacro(<< this->GetClassName() << "::" << this->GetClassName() << "()");

}


/*!
 *
 */
svkAlgorithmPortMapper::~svkAlgorithmPortMapper()
{
    vtkDebugMacro(<<this->GetClassName()<<"::~svkAlgorithmPortMapper()");
}


/*!
 * Sets the algorithm that the inputs will be mapped to.
 */
void svkAlgorithmPortMapper::SetAlgorithm( vtkAlgorithm* algo )
{
    this->algo = algo;
}


/*!
 *  This method initializes a given input port. This MUST be called in the constructor
 *  of the subclass, and only there before setting any of the inputs.
 */
void svkAlgorithmPortMapper::InitializeInputPort( int port, string name, int type, bool required )
{
    // Only initialize a given input port parameter once.
    if( this->GetAlgorithmInputPort( port ) == NULL ) {
        // Make sure the parameter name array is large enough to hold the new name
        if( this->inputPortTypes.size() != this->algo->GetNumberOfInputPorts()) {
            this->inputPortTypes.resize( this->algo->GetNumberOfInputPorts() );
        }
        if( this->inputPortNames.size() != this->algo->GetNumberOfInputPorts()) {
            this->inputPortNames.resize( this->algo->GetNumberOfInputPorts() );
        }
        if( this->inputPortRequired.size() != this->algo->GetNumberOfInputPorts()) {
            this->inputPortRequired.resize( this->algo->GetNumberOfInputPorts() );
        }
        this->inputPortTypes[port]    = type;
        this->inputPortNames[port]    = name;
        this->inputPortRequired[port] = required;
        // TODO: Remove this logic once optional ports have been setup
        if( type == SVK_BOOL) {
            svkBool* inputBool = svkBool::New();
            inputBool->SetValue(false);
            this->SetAlgorithmInputPort(port, inputBool);
            inputBool->Delete();
        }
    }
}


/*!
 * Extracts input port parameters from an XML element and maps them to the internal algorithm.
 */
void svkAlgorithmPortMapper::SetInputPortsFromXML( vtkXMLDataElement* element )
{
    vtkIndent indent;
    if( element != NULL ) {
        for( int i = 0; i < this->algo->GetNumberOfInputPorts(); i++ ) {
            vtkXMLDataElement* parameterElement =  element->FindNestedElementWithName(this->GetInputPortName(i).c_str());
            if( parameterElement != NULL ) {
                string parameterStringValue = string(parameterElement->GetCharacterData());
                int dataType = this->GetInputPortType( i );
                if( dataType == SVK_DOUBLE ) {
                    this->SetDoubleInputPortValue(i, svkUtils::StringToDouble( parameterStringValue ));
                } else if ( dataType == SVK_INT ) {
                    this->SetIntInputPortValue(i, svkUtils::StringToInt( parameterStringValue ));
                } else if ( dataType == SVK_STRING ) {
                    this->SetStringInputPortValue(i, parameterStringValue );
                } else if ( dataType == SVK_BOOL ) {
                    //TODO: How should bools be handled in the XML???
                    this->SetBoolInputPortValue(i, true );
                } else if ( dataType == SVK_MR_IMAGE_DATA ) {
                    this->SetMRImageInputPortValue( i, parameterStringValue );
                } else if ( dataType == SVK_XML ) {
                    this->SetXMLInputPortValue( i, parameterElement );
                }
            }
        }
    }
}


/*!
 * Fills the input port information for all the input ports.
 */
int svkAlgorithmPortMapper::FillInputPortInformation( int port, vtkInformation* info )
{
    if( port < this->algo->GetNumberOfInputPorts() ) {
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
                svkAlgorithmPortMapper::GetClassTypeFromDataType(this->inputPortTypes[port]).c_str());
        if( !this->inputPortRequired[port] ) {
            info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(),1);
        }
    }
    return 1;
}


/*!
 * Parameter port setter.
 */
void svkAlgorithmPortMapper::SetDoubleInputPortValue( int port, double value )
{
    if( this->GetInputPortType(port) == SVK_DOUBLE ) {
        vtkDataObject* parameter =  this->GetAlgorithmInputPort( port );
        if( parameter == NULL ) {
            parameter = svkDouble::New();
            this->SetAlgorithmInputPort(port, parameter);
            parameter->Delete();
            parameter =  this->GetAlgorithmInputPort( port );
        }
        svkDouble::SafeDownCast( parameter )->SetValue(value);
    } else {
        cerr << "ERROR: Input parameter port type mismatch! Port " << port << " is not of type double. " << endl;
    }
}


/*!
 * Parameter port getter.
 */
svkDouble* svkAlgorithmPortMapper::GetDoubleInputPortValue( int port )
{
    if( this->GetInputPortType(port) == SVK_DOUBLE ) {
        vtkDataObject* parameter =  this->GetAlgorithmInputPort( port );
        return svkDouble::SafeDownCast( parameter );
    } else {
        cerr << "ERROR: Input parameter port type mismatch! Port " << port << " is not of type double. " << endl;
    }
}


/*!
 * Parameter port setter.
 */
void svkAlgorithmPortMapper::SetIntInputPortValue( int port, int value )
{
    // CHECK THAT THIS IS THE CORRECT TYPE FIRS
    if( this->GetInputPortType(port) == SVK_INT ) {
        vtkDataObject* parameter =  this->GetAlgorithmInputPort( port );
        if( parameter == NULL ) {
            parameter = svkInt::New();
            this->SetAlgorithmInputPort(port, parameter);
            parameter->Delete();
            parameter =  this->GetAlgorithmInputPort( port );
        }
        svkInt::SafeDownCast( parameter )->SetValue(value);
    } else {
        cerr << "ERROR: Input parameter port type mismatch! Port " << port << " is not of type int. " << endl;
    }
}


/*!
 * Parameter port getter.
 */
svkInt* svkAlgorithmPortMapper::GetIntInputPortValue( int port )
{
    if( this->GetInputPortType(port) == SVK_INT ) {
        vtkDataObject* parameter =  this->GetAlgorithmInputPort( port );
        return svkInt::SafeDownCast( parameter );
    } else {
        cerr << "ERROR: Input parameter port type mismatch! Port " << port << " is not of type int. " << endl;
    }
}


/*!
 * Parameter port setter.
 */
void svkAlgorithmPortMapper::SetStringInputPortValue( int port, string value )
{
    if( this->GetInputPortType(port) == SVK_STRING ) {
        vtkDataObject* parameter =  this->GetAlgorithmInputPort( port );
        if( parameter == NULL ) {
            parameter = svkString::New();
            this->SetAlgorithmInputPort(port, parameter);
            parameter->Delete();
            parameter =  this->GetAlgorithmInputPort( port );
        }
        svkString::SafeDownCast( parameter )->SetValue(value);
    } else {
        cerr << "ERROR: Input parameter port type mismatch! Port " << port << " is not of type string. " << endl;
    }
}


/*!
 * Parameter port getter.
 */
svkString* svkAlgorithmPortMapper::GetStringInputPortValue( int port )
{
    if( this->GetInputPortType(port) == SVK_STRING ) {
        vtkDataObject* parameter =  this->GetAlgorithmInputPort( port );
        return svkString::SafeDownCast( parameter );
    } else {
        cerr << "ERROR: Input parameter port type mismatch! Port " << port << " is not of type string. " << endl;
    }
}


/*!
 * Parameter port setter.
 */
void svkAlgorithmPortMapper::SetBoolInputPortValue( int port, bool value )
{
    if( this->GetInputPortType(port) == SVK_BOOL ) {
        vtkDataObject* parameter =  this->GetAlgorithmInputPort( port );
        if( parameter == NULL ) {
            parameter = svkBool::New();
            this->SetAlgorithmInputPort(port, parameter);
            parameter->Delete();
            parameter =  this->GetAlgorithmInputPort( port );
        }
        svkBool::SafeDownCast( parameter )->SetValue(value);
    } else {
        cerr << "ERROR: Input parameter port type mismatch! Port " << port << " is not of type Bool. " << endl;
    }
}


/*!
 * Parameter port getter.
 */
svkBool* svkAlgorithmPortMapper::GetBoolInputPortValue( int port )
{
    if( this->GetInputPortType(port) == SVK_BOOL ) {
        vtkDataObject* parameter =  this->GetAlgorithmInputPort( port );
        return svkBool::SafeDownCast( parameter );
    } else {
        cerr << "ERROR: Input parameter port type mismatch! Port " << port << " is not of type Bool. " << endl;
    }
}


/*!
 * Parameter port setter.
 */
void svkAlgorithmPortMapper::SetXMLInputPortValue( int port, vtkXMLDataElement* value )
{
    if( this->GetInputPortType(port) == SVK_XML ) {
        vtkDataObject* parameter =  this->GetAlgorithmInputPort( port );
        if( parameter == NULL ) {
            parameter = svkXML::New();
            this->SetAlgorithmInputPort(port, parameter);
            parameter->Delete();
            parameter = this->GetAlgorithmInputPort( port );
        }
        svkXML::SafeDownCast( parameter )->SetValue(value);
    } else {
        cerr << "ERROR: Input parameter port type mismatch! Port " << port << " is not of type XML." << endl;
    }
}


/*!
 * Parameter port getter.
 */
svkXML* svkAlgorithmPortMapper::GetXMLInputPortValue( int port )
{
    if( this->GetInputPortType(port) == SVK_XML ) {
        vtkDataObject* parameter =  this->GetAlgorithmInputPort( port );
        return svkXML::SafeDownCast( parameter );
    } else {
        cerr << "ERROR: Input parameter port type mismatch! Port " << port << " is not of type XML. " << endl;
    }
}


/*!
 * Sets an MRI image port. Input is a filename and a reader will be instantiated to read the file.
 */
void svkAlgorithmPortMapper::SetMRImageInputPortValue( int port, string filename )
{
    if( this->GetInputPortType(port) == SVK_MR_IMAGE_DATA ) {
        // READ THE IMAGE
        svkImageReaderFactory* readerFactory = svkImageReaderFactory::New();
        svkImageReader2* reader = readerFactory->CreateImageReader2(filename.c_str());
        readerFactory->Delete();
        if (reader != NULL) {
            reader->SetFileName( filename.c_str() );
            reader->Update();
            this->SetAlgorithmInputPort(port, reader->GetOutput() );
            reader->Delete();
        }
    } else {
        cerr << "ERROR: Input parameter port type mismatch! Port " << port << " is not of type svkMriImageData. " << endl;
    }
}


/*!
 * Simple input port getter.
 */
svkMriImageData* svkAlgorithmPortMapper::GetMRImageInputPortValue( int port )
{
    if( this->GetInputPortType(port) == SVK_MR_IMAGE_DATA ) {
        return svkMriImageData::SafeDownCast( this->GetAlgorithmInputPort(port) );
    } else {
        cerr << "ERROR: Input parameter port type mismatch! Port " << port << " is not of type svkMriImageData. " << endl;
        return NULL;
    }
}


/*!
 *  This method returns the classname associated with a given data type.
 */
string svkAlgorithmPortMapper::GetClassTypeFromDataType( int type )
{
    string classType;
    if( type == SVK_DOUBLE ) {
        classType = "svkDouble";
    } else if ( type == SVK_INT ) {
        classType = "svkInt";
    } else if ( type == SVK_STRING ) {
        classType = "svkString";
    } else if ( type == SVK_BOOL ) {
        classType = "svkBool";
    } else if ( type == SVK_MR_IMAGE_DATA ) {
        classType = "svkMriImageData";
    } else if ( type == SVK_XML ) {
        classType = "svkXML";
    }
    return classType;
}


/*!
 * Returns the name of the given parameter port.
 */
string svkAlgorithmPortMapper::GetInputPortName( int port )
{
    string parameterName;
    if( port >= 0 && port < this->algo->GetNumberOfInputPorts() ) {
        parameterName = this->inputPortNames[port];
    } else {
        cout << "ERROR: port " << port << " is not an input parameter port!" << endl;
    }
    return parameterName;
}


/*!
 *  PrintSelf method calls parent class PrintSelf, then prints all parameters.
 *
 */
void svkAlgorithmPortMapper::PrintSelf( ostream &os, vtkIndent indent )
{
    Superclass::PrintSelf( os, indent );
    os << indent << "Svk Parameters:" << endl;
    indent = indent.GetNextIndent();
    for( int i = 0; i < this->algo->GetNumberOfInputPorts(); i++ ) {
        vtkDataObject* parameterObject =  this->GetAlgorithmInputPort( i );
        if( svkString::SafeDownCast(parameterObject) != NULL ) {
            os << indent << this->GetInputPortName(i) << ": " << svkString::SafeDownCast(parameterObject)->GetValue() << endl;
        } else if ( svkDouble::SafeDownCast(parameterObject) != NULL ) {
            os << indent << this->GetInputPortName(i) << ": " << svkDouble::SafeDownCast(parameterObject)->GetValue() << endl;
        } else if ( svkInt::SafeDownCast(parameterObject) != NULL ) {
            os << indent << this->GetInputPortName(i) << ": " << svkInt::SafeDownCast(parameterObject)->GetValue() << endl;
        } else if ( svkBool::SafeDownCast(parameterObject) != NULL ) {
            os << indent << this->GetInputPortName(i) << ": " << svkInt::SafeDownCast(parameterObject)->GetValue() << endl;
        } else if ( svkImageData::SafeDownCast(parameterObject) != NULL ) {
            string filename = "FILENAME UNKNOWN";
            if( svkImageData::SafeDownCast(parameterObject)->GetSourceFileName() != NULL ){
                filename = svkImageData::SafeDownCast(parameterObject)->GetSourceFileName();
            }
            os << indent << this->GetInputPortName(i) << ": " << filename << endl;
        } else {
            os << indent << this->GetInputPortName(i) << ": NOT SET" << endl;
        }
    }
}


/*!
 * Returns the port number for the given parameter name. Returns -1 if the port does not exist.
 */
int svkAlgorithmPortMapper::GetInputPortNumber( string name )
{
    int port = -1;
    for( int i = 0; i < this->algo->GetNumberOfInputPorts(); i++ ) {
        if( name == this->inputPortNames[i]) {
            port = i;
        }
    }
    if( port == -1 ) {
        cout << "ERROR: No input parameter port named " << name << " found!" << endl;
    }
    return port;
}


/*!
 * Returns the type of a given input port parameter.
 */
int svkAlgorithmPortMapper::GetInputPortType( int port )
{
    if( port < this->inputPortTypes.size()) {
        return this->inputPortTypes[port];
    } else {
        return -1;
    }
}

/*!
 * Gets a data object from the input port.
 */
vtkDataObject* svkAlgorithmPortMapper::GetAlgorithmInputPort( int port )
{
    return this->algo->GetInputDataObject(port, 0);

}


/*!
 * Gets a data object into the input port.
 */
vtkDataObject* svkAlgorithmPortMapper::SetAlgorithmInputPort( int port, vtkDataObject* input )
{
    if(input) {
      this->algo->SetInputConnection(port, input->GetProducerPort());
    } else {
      // Setting a NULL input removes the connection.
      this->algo->SetInputConnection(port, 0);
    }

}
