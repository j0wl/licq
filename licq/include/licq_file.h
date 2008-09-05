#ifndef INIFILE_H
#define INIFILE_H

#include <string>

// Define for marking functions as deprecated
#ifndef LICQ_DEPRECATED
# if defined(__GNUC__) && !defined(__INTEL_COMPILER) && (__GNUC__ - 0 > 3 || (__GNUC__ - 0 == 3 && __GNUC_MINOR__ - 0 >= 2))
#  define LICQ_DEPRECATED __attribute__ ((__deprecated__))
# elif defined(_MSC_VER) && (_MSC_VER >= 1300)
#  define LICQ_DEPRECATED __declspec(deprecated)
# else
#  define LICQ_DEPRECATED
# endif
#endif

#define MAX_SECTIONxNAME_LEN 160
#define MAX_KEYxNAME_LEN 160
#define MAX_LINE_LEN 4096

const unsigned short INI_FxWARN           = 0x0001;
const unsigned short INI_FxERROR          = 0x0002;
const unsigned short INI_FxFATAL          = 0x0004;
const unsigned short INI_FxALLOWxCREATE   = 0x0008;

const unsigned short INI_ExNOKEY          = 0x0001;
const unsigned short INI_ExNOSECTION      = 0x0002;
const unsigned short INI_ExFORMAT         = 0x0004;
const unsigned short INI_ExIOREAD         = 0x0008;
const unsigned short INI_ExIOWRITE        = 0x0010;

void AddNewLines(char *_szDest, const char *_szSource);

//=====CIniFile================================================================
class CIniFile
{
public:
  CIniFile(unsigned short _nFlags = 0);
  ~CIniFile();

  bool LoadFile(const char *_szFilename);
  bool ReloadFile();
  bool FlushFile();
  void CloseFile();
  void SetFileName(const char *_szFilename);

  // Flag setting and clearing
  void SetFlags(unsigned short _nFlags)
    { m_nFlags = _nFlags; };
  void SetFlag(unsigned short _nFlag)
    { m_nFlags |= _nFlag; };
  void ClearFlag(unsigned short _nFlag)
    { m_nFlags &= ~_nFlag; };
  unsigned short GetFlags()
    { return (m_nFlags); };
  bool GetFlag(unsigned short _nFlag)
    { return (m_nFlags & _nFlag); };

  bool SetSection(const char *_szSectionName);
  bool CreateSection(const char *_szSectionName);
  bool ReadStr(const std::string& Key, char* data, const char* defValue = NULL, bool trim = true, int maxLength = 0);
  bool ReadNum(const std::string& key, unsigned int &data, unsigned int defValue = 0);
  bool ReadNum(const std::string& key, signed int &data, signed int defValue = 0);
  bool ReadNum(const std::string& key, unsigned short &data, unsigned short defValue = 0);
  bool ReadNum(const std::string& key, signed short &data, signed short defValue = 0);
  bool ReadNum(const std::string& key, char &data, char defValue = 0);
  bool ReadBool(const std::string& key, bool &data, const bool defValue = false);

  bool WriteStr(const std::string& key, const char* data);
  bool WriteNum(const std::string& key, unsigned int data);
  bool WriteNum(const std::string& key, signed int data);
  bool WriteNum(const std::string& key, unsigned short data);
  bool WriteNum(const std::string& key, signed short data);
  bool WriteNum(const std::string& key, char data);
  bool WriteBool(const std::string& key, bool data);

  int Error()  { return (m_nError); }
  const char *FileName()  { return m_szFilename; }

  // Long differs in size between 32 bit systems and 64 bit systems so int
  // should be used instead. This function is kept to keep old code working but
  // since it is unlikely that we need to save any 64 bit integers these
  // function is declared as deprecated.
  LICQ_DEPRECATED bool ReadNum(const std::string& key, unsigned long &data, unsigned long defValue = 0);
  LICQ_DEPRECATED bool WriteNum(const std::string& key, unsigned long data);

protected:
  // Data members
  char *m_szFilename,
       *m_szBuffer;
  int m_nBufPos,
      m_nBufSize,
      m_nError;
  char *m_szSectionName;
  int m_nSectionStart,
      m_nSectionEnd;
  unsigned short m_nFlags;
  bool //m_bEof,
       //m_bEos,
       m_bChanged;

  // Functions
  char *ReadLine(char *_szDest, int nDestSize);
  char *GetSectionFromLine(char *_szBuffer, const char *_szLine);
  char *GetKeyFromLine(char *_szBuffer, const char *_szLine);
  char *GetDataFromLine(char *_szBuffer, const char *_szLine,
                        bool bTrim = true, int _nMax = 0);
  void Warn(int nError, const char *_sz = NULL);
  void InsertStr(const char *_szNewStr, int _nCutStart, int _nCutEnd);

  /*bool Eof()
    { return (m_bEof); };
  bool Eos()
    { return (m_bEos); };*/
  void ResetFile();
  void ResetSection();
};


#endif
