#pragma once

// Sample Usage:
//
//     //
//     // Initialization
//     //
//
//     using namespace Stardust;
//
//     #define INCOMING_RING_BUFFER_SIZE (1024*16) // or whatever
//
//     CRingBuffer m_ringbuf;
//     m_ringbuf.Create( INCOMING_RING_BUFFER_SIZE + 1 );
//
//     char m_chInBuf[ INCOMING_BUFFER_SIZE + 1 ];
//
//     ...
//
//     //
//     // Then later, upon receiving data...
//     //
//
//     int iNumberOfBytesRead = 0;
// 
//     while( READ_INCOMING(m_chInBuf,INCOMING_BUFFER_SIZE,&iNumberOfBytesRead,0)
//     && iNumberOfBytesRead > 0 )
//     {
//          // add incoming data to the ring buffer
//          m_ringbuf.WriteBinary(m_chInBuf,iNumberOfBytesRead);
//
//          // pull it back out one line at a time, and distribute it
//          CString strLine;
//          while( m_ringbuf.ReadTextLine(strLine) )
//          {
//              strLine.TrimRight("\r\n");
//
//              if( !ON_INCOMING( strLine ) )
//                  return false;
//          }
//     }
//
//     // Fall out, until more incoming data is available...
//
// Notes:
//     In the above example, READ_INCOMING and ON_INCOMING are
//     placeholders for functions, that read and accept incoming data,
//     respectively.
//
//     READ_INCOMING receives raw data from the socket.
//
//     ON_INCOMING accepts incoming data from the socket, one line at
//     a time.
// 
//////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

///////////////////////////////////////////////////////////////////////////////
// Stardust Namespace
///////////////////////////////////////////////////////////////////////////////

class CRingBuffer  
{
protected:
	///////////////////////////////////////////////////////////////////
	// Protected Member Variables
	//
	unsigned char * m_pBuf;
	unsigned char * m_pTmpBuf; // dupe buf for working so no alloc/free while running.
	int m_nBufSize;   // the size of the ring buffer
	int m_iReadPtr;   // the read pointer
	int m_iWritePtr;  // the write pointer

public:
	///////////////////////////////////////////////////////////////////
	// Constructor
	//
	CRingBuffer()
	{
		m_pBuf = NULL;
		m_pTmpBuf = NULL;
		m_nBufSize = 0;
		m_iReadPtr = 0;
		m_iWritePtr = 0;
	}

	///////////////////////////////////////////////////////////////////
	// Destructor
	//
	virtual ~CRingBuffer()
	{
		Destroy();
	}

	///////////////////////////////////////////////////////////////////
	// Method: Create
	// Purpose: Initializes the ring buffer for use.
	// Parameters:
	//     [in] iBufSize -- maximum size of the ring buffer
	// Return Value: true if successful, otherwise false.
	//
	bool Create( int iBufSize )
	{
		bool bResult = false;
		{
			m_pBuf = new unsigned char[ iBufSize ];
			if( m_pBuf )
			{
				m_nBufSize = iBufSize;
				memset( m_pBuf, 0, m_nBufSize );

				m_pTmpBuf = new unsigned char[ iBufSize ];
				if( m_pTmpBuf )
				{
					memset( m_pTmpBuf, 0, m_nBufSize );
					bResult = true;
				}
			}
		}
		return bResult;
	}

	///////////////////////////////////////////////////////////////////
	// Method: Destroy
	// Purpose: Cleans up ring buffer by freeing memory and resetting
	//     member variables to original state.
	// Parameters: (None)
	// Return Value: (None)
	//
	void Destroy()
	{
		if( m_pBuf )
			delete m_pBuf;

		if( m_pTmpBuf )
			delete m_pTmpBuf;

		m_pBuf = NULL;
		m_pTmpBuf = NULL;
		m_nBufSize = 0;
		m_iReadPtr = 0;
		m_iWritePtr = 0;
	}


	void Empty()
	{
		m_iReadPtr = 0;
		m_iWritePtr = 0;
	}
	///////////////////////////////////////////////////////////////////
	// Method: GetMaxReadSize
	// Purpose: Returns the amount of data (in bytes) available for
	//     reading from the buffer.
	// Parameters: (None)
	// Return Value: Amount of data (in bytes) available for reading.
	//
	int GetMaxReadSize()
	{
		if( m_pBuf )
		{
			if( m_iReadPtr == m_iWritePtr )
				return 0;

			if( m_iReadPtr < m_iWritePtr )
				return m_iWritePtr - m_iReadPtr;

			if( m_iReadPtr > m_iWritePtr )
				return (m_nBufSize-m_iReadPtr)+m_iWritePtr;
		}
		return 0;
	}

	///////////////////////////////////////////////////////////////////
	// Method: GetMaxWriteSize
	// Purpose: Returns the amount of space (in bytes) available for
	//     writing into the buffer.
	// Parameters: (None)
	// Return Value: Amount of space (in bytes) available for writing.
	//
	int GetMaxWriteSize()
	{
		if( m_pBuf )
		{
			if( m_iReadPtr == m_iWritePtr )
				return m_nBufSize;

			if( m_iWritePtr < m_iReadPtr )
				return m_iReadPtr - m_iWritePtr;

			if( m_iWritePtr > m_iReadPtr )
				return (m_nBufSize-m_iWritePtr)+m_iReadPtr;
		}
		return 0;
	}

	///////////////////////////////////////////////////////////////////
	// Method: WriteBinary
	// Purpose: Writes binary data into the ring buffer.
	// Parameters:
	//     [in] pBuf - Pointer to the data to write.
	//     [in] nBufLen - Size of the data to write (in bytes).
	// Return Value: true upon success, otherwise false.
	// 
	bool WriteBinary( unsigned char * pBuf, int nBufLen )
	{
		bool bResult = false;
		{
			if( nBufLen < GetMaxWriteSize() )
			{
				// easy case, no wrapping
				if( m_iWritePtr + nBufLen < m_nBufSize )
				{
					memcpy( &m_pBuf[m_iWritePtr], pBuf, nBufLen );
					m_iWritePtr += nBufLen;
				}
				else // harder case we need to wrap
				{
					int iFirstChunkSize = m_nBufSize - m_iWritePtr;
					int iSecondChunkSize = nBufLen - iFirstChunkSize;

					memcpy( &m_pBuf[m_iWritePtr], pBuf, iFirstChunkSize );
					memcpy( &m_pBuf[0], &pBuf[iFirstChunkSize], iSecondChunkSize );

					m_iWritePtr = iSecondChunkSize;
				}
				bResult =true;
			}
		}
		return bResult;
	}

	///////////////////////////////////////////////////////////////////
	// Method: ReadBinary
	// Purpose: Reads (and extracts) data from the ring buffer.
	// Parameters:
	//     [in/out] pBuf - Pointer to where read data will be stored.
	//     [in] nBufLen - Size of the data to be read (in bytes).
	// Return Value: true upon success, otherwise false.
	// 
	bool ReadBinary( unsigned char * pBuf, int nBufLen )
	{
		bool bResult = false;
		{
			if( nBufLen <= GetMaxReadSize() )
			{
				// easy case, no wrapping
				if( m_iReadPtr + nBufLen <= m_nBufSize )
				{
					memcpy( pBuf, &m_pBuf[m_iReadPtr], nBufLen );
					m_iReadPtr += nBufLen;
				}
				else // harder case, buffer wraps
				{
					int iFirstChunkSize = m_nBufSize - m_iReadPtr;
					int iSecondChunkSize = nBufLen - iFirstChunkSize;

					memcpy( pBuf, &m_pBuf[m_iReadPtr], iFirstChunkSize );
					memcpy( &pBuf[iFirstChunkSize], &m_pBuf[0], iSecondChunkSize );

					m_iReadPtr = iSecondChunkSize;
				}
				bResult =true;
			}
		}
		return bResult;
	}

	///////////////////////////////////////////////////////////////////
	// Method: PeekChar
	// Purpose: Peeks at a character at the given position in the ring
	//     buffer, without extracting it.
	// Parameters:
	//     [in] iPos - Index of the character to peek (zero-based).
	//     [out] ch - The character peeked.
	// Return Value: true upon success, otherwise false.
	// 
	bool PeekChar( int iPos, unsigned char & ch )
	{
		bool bResult = false;
		{
			if( iPos < GetMaxReadSize() )
			{
				if( m_iWritePtr > m_iReadPtr )
				{
					// easy case, buffer doesn't wrap
					ch = m_pBuf[ m_iReadPtr+iPos ];
					bResult = true;
				}
				else if( m_iWritePtr == m_iReadPtr )
				{
					// nothing in the buffer
				}
				else if( m_iWritePtr < m_iReadPtr )
				{
					// harder case, buffer wraps

					if( m_iReadPtr + iPos < m_nBufSize )
					{
						// pos was in first chunk
						ch = m_pBuf[ m_iReadPtr + iPos ];
						bResult = true;
					}
					else
					{
						// pos is in second chunk
						ch = m_pBuf[ iPos - (m_nBufSize-m_iReadPtr) ];
						bResult = true;
					}
				}
			}
		}
		return bResult;
	}

	///////////////////////////////////////////////////////////////////
	// Method: FindChar
	// Purpose: Determines if the specified character is in the ring
	//     buffer, and if so, returns the index position.
	// Parameters:
	//     [in] chLookFor - Character to look for in the ring buffer.
	//     [out] riPos - The index position of the character, if found.
	// Return Value: true upon success, otherwise false.
	// 
	bool FindChar( unsigned char chLookFor, int & riPos )
	{
		bool bResult = false;
		{
			int iSize = GetMaxReadSize();

			for( int i = 0; i < iSize; i++ )
			{
				unsigned char ch;
				if( PeekChar( i, ch ) )
				{
					if( ch == chLookFor )
					{
						riPos = i;
						bResult = true;
						break;
					}
				}
			}
		}
		return bResult;
	}	
};




