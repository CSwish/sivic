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


#ifndef SVK_PACS_INTERFACE_H
#define SVK_PACS_INTERFACE_H


#include <vtkObject.h>
#include <vtkObjectFactory.h>
#include <vector>
#include <svkTypes.h>

namespace svk {


using namespace std;


/*!
 *  A generic interface to PACS. Currently only has methods to connect, send
 *  images and disconnect.
 */
class svkPACSInterface : public vtkObject 
{

    // if these are accessed only via the corresponding controller, then these don't need to be public
    public:

        vtkTypeRevisionMacro( svkPACSInterface, vtkObject);

        //! Create a connection to PACS. Return true if the connection can be made.        
        virtual bool                    Connect() = 0;

        //! Send a set of images to PACS. Returns true if the images can be sent.       
        virtual bool                    SendImagesToPACS( string sourceDirectory, svkTypes::AnatomyType anatomyType ) = 0;

        //! Close the PACS connection. Returns true if the connection closes cleanly.       
        virtual bool                    Disconnect() = 0;

        //! Get a string representation of the PACS connection.       
        virtual string                  GetPACSTargetString();

        //! Set the string representation of the PACS connection.       
        virtual void                    SetPACSTargetString( string pacsTarget );

    protected:

        string pacsTarget;

        svkPACSInterface();
        ~svkPACSInterface();

};


}   //svk


#endif //SVK_PACS_INTERFACE_H

