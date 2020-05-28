#ifndef MOVIEFILE_H_
#define MOVIEFILE_H_

#define MAX_MOVIE_BUFFER_LEN	50120
#define DEFAULT_LOAD_BUFFER_LEN (1024*8)
/** Movie File error indicators.
*/
enum MovieFileIoStatus
{
	MOVIEFILE_SUCCESS = 0,
	MOVIEFILE_EOF = -100,
	MOVIEFILE_CRITICAL_READERROR,
};

/** Picture types.
*/
enum 
{
	UNKNOWN_VOP=-1,
	I_VOP=0x0, // intra-coded
	P_VOP=0x1, // predictive-coded
	B_VOP=0x2, // bidirectionally-predictive-coded
	S_VOP=0x3, // sprite
};

enum
{
	FILECTRL_OPEN_READ	= 0x1,
	FILECTRL_OPEN_WRITE = 0x2,
};
/** file operations abstraction layer */
class FileCtrl
{
public:
	/** open file */
	virtual int	Open(char * pathname, int filesize, int mode)=0;
	/** close file */
	virtual int Close()=0;
	/** read data into the buf */
	virtual int Read(char * buf, int nbytes)=0;
	/** write data into the file */
	virtual int Write(char * buf, int nbytes)=0;
	/** set file reading/writing position */
	virtual int Seek(int pos)=0;
	/** return file reading/writing position */
	virtual int Tell()=0;
	/** return file size */
	virtual int Size()=0;
	/** return if is streaming mode */
	virtual int IsStreaming(){
		return 0;
	}

	virtual ~FileCtrl() {};
};


FileCtrl * CreateFileCtrl();

/** MovieFile: abstracting input/ouput operations for mpeg4 iso file de/mux.
*/
class MovieFile
{
public:
	/** if you have multibyte character system implement constructor with FileCtrl instead */
	MovieFile(char * pathname);
	/** NOTE: pFileCtrl is deleted by ~MovieFile() */
	MovieFile(FileCtrl * pFileCtrl);
	virtual ~MovieFile();

	// read data for ISO file parsing.
	/** return 8 bits */
	char  		GetChar();
	/** return 8 bits in int format. return -1 for error */
	int  			GetC();
	/** return buffer size amount */
	int   		GetBuffer(char * pBuf, int size);
	/** return 16 bits */
	short 		Get16();
	/** return 32 bits */
	int   		Get32();
	/** return 24 bits */
	int   		Get24();
#ifndef TI_ARM	
#if defined(__linux__)
	long long       Get64();
#else
	__int64 	Get64();
#endif
#else
	int64s 		Get64();
#endif 	
	// bit handling to support mp4 video element stream parsing.
	/** return bits iBits amount upto 32 bits(cosume bits) */
	int			GetBits(int iBits);
	/** return bits iBits amount upto 32 bits(doesnot cosume bits) */
	int			NextBits(int iBits);
	/** move to next start code */
	int			GetStartCode();
	/** byte align first,move to next start code */
	int			NextStartCode();
#if 0
	/** return enum picture type. */
	int			SearchNextVop(int & iPos);
#endif
	// generic file operations.
	/** return read pointer */
	virtual int GetPos();
	/** set read pointer */
	virtual int SetPos(int pos);
	/** return file size */
	virtual int GetFileSize();

	// data conversion utilities.
	/** convert buf to integer in big endian */
	static int 	 GetIntB(char * cBuf);
	/** convert buf to integer in little endian */
	static int   GetIntL(char * cBuf);
	/** convert buf to short in big endian */
	static short GetShortB(char * cBuf);
	/** convert buf to short in little endian */
	static short GetShortL(char * cBuf);
	/** stored data's endianness */
	void SetReadLittleEndian(bool bEnable)
	{
		m_bReadLittleEndian = bEnable;
	}

protected:
	MovieFile() 
	{
		m_bDeleteFileCtrl = false;
		// MovieFile requires file name
	}
	virtual int FillBuffer(char *pBuf = NULL, unsigned int nFillSize = DEFAULT_LOAD_BUFFER_LEN);
	void  		Reset();
	void		Initialize();

	char *	m_cBuffer;
	char *	m_pBufferBegin;	
	char *	m_pBufferEnd;	
	int		m_iFileSize;
	int		m_iFilePos;

	FileCtrl * m_pFileCtrl;

	char				m_iRemainBits;
	unsigned char		m_CurChar;

	MovieFileIoStatus	m_IoStatus;

	bool m_bReadLittleEndian;
	bool m_bDeleteFileCtrl;
};

#endif

