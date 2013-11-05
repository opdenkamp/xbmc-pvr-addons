/***************************************************************************
 * Copyright (C) 2012 Marcus Efraimsson.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 *
 ***************************************************************************/

#pragma once

#include <string>

/**
  * Namespace for serialization specific functionality in the DVBLink Remote API library.
  */
namespace dvblinkremoteserialization {
  /**
    * A constant string representing the XML declaration part for XML data used in all requests 
    * in the DVBLink Remote API library.
    */
  const std::string DVBLINK_REMOTE_SERIALIZATION_XML_DECLARATION = "xml version=\"1.0\" encoding=\"utf-8\" ";

  /**
    * A constant string representing the xmlns:i namespace of the XML root node part off every 
    * requests made in the DVBLink Remote API library.
    */
  const std::string DVBLINK_REMOTE_SERIALIZATION_XML_I_NAMESPACE = "http://www.w3.org/2001/XMLSchema-instance";

  /**
    * A constant string representing the xmlns namespace of the XML root node part off every 
    * requests made in the DVBLink Remote API library.
    */
  const std::string DVBLINK_REMOTE_SERIALIZATION_XML_NAMESPACE = "http://www.dvblogic.com";
};
