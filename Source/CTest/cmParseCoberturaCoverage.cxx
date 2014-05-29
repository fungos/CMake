#include "cmStandardIncludes.h"
#include "cmSystemTools.h"
#include "cmXMLParser.h"
#include "cmParseCoberturaCoverage.h"
#include <cmsys/Directory.hxx>
#include <cmsys/FStream.hxx>

//----------------------------------------------------------------------------
class cmParseCoberturaCoverage::XMLParser: public cmXMLParser
{
public:
  XMLParser(cmCTest* ctest, cmCTestCoverageHandlerContainer& cont)
    : CTest(ctest), Coverage(cont)
  {
   this->InSources = false;
   this->InSource  = false;
   this->FilePaths.push_back(this->Coverage.SourceDir);
   this->CurFileName = "";
  }

  virtual ~XMLParser()
  {
  }

protected:


  virtual void EndElement(const std::string& name)
  {
   if(name == "source")
     {
     this->InSource=false;
     }
   else if (name == "sources")
     {
     this->InSources=false;
     }
  }

  virtual void CharacterDataHandler(const char* data, int length)
  {
     std::string tmp;
     tmp.insert(0,data,length);
     if (this->InSources && this->InSource)
       {
       this->FilePaths.push_back(tmp);
       cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT, "Adding Source: "
                   << tmp << std::endl);
       }
  }

  virtual void StartElement(const std::string& name, const char** atts)
  {
    std::string FoundSource;
    std::string finalpath = "";
    if(name == "source")
    {
      this->InSource = true;
    }
    else if(name == "sources")
      {
      this->InSources = true;
      }
    else if(name == "class")
    {
      int tagCount = 0;
      while(true)
      {
        if(strcmp(atts[tagCount], "filename") == 0)
        {
          cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT, "Reading file: "
                     << atts[tagCount+1]<< std::endl);
          std::string filename = atts[tagCount+1];
          for(size_t i=0;i < FilePaths.size();i++)
            {
            finalpath = FilePaths[i] + "/" + filename;
            if(cmSystemTools::FileExists(finalpath.c_str()))
              {
              this->CurFileName = finalpath;
              break;
              }
            }
          cmsys::ifstream fin(this->CurFileName.c_str());
          if(this->CurFileName == "" || !fin )
          {
            this->CurFileName = this->Coverage.BinaryDir + "/" +
                                   atts[tagCount+1];
            fin.open(this->CurFileName.c_str());
            if (!fin)
            {
              cmCTestLog(this->CTest, ERROR_MESSAGE,
                         "Python Coverage: Error opening " << this->CurFileName
                         << std::endl);
              this->Coverage.Error++;
              break;
            }
          }
          std::string line;
          FileLinesType& curFileLines =
            this->Coverage.TotalCoverage[this->CurFileName];
          curFileLines.push_back(-1);
          while(cmSystemTools::GetLineFromStream(fin, line))
          {
            curFileLines.push_back(-1);
          }

          break;
        }
        ++tagCount;
      }
    }
    else if(name == "line")
    {
      int tagCount = 0;
      int curNumber = -1;
      int curHits = -1;
      while(true)
      {
        if(strcmp(atts[tagCount], "hits") == 0)
        {
          curHits = atoi(atts[tagCount+1]);
        }
        else if(strcmp(atts[tagCount], "number") == 0)
        {
          curNumber = atoi(atts[tagCount+1]);
        }

        if(curHits > -1 && curNumber > 0)
        {
          FileLinesType& curFileLines =
            this->Coverage.TotalCoverage[this->CurFileName];
            {
            curFileLines[curNumber-1] = curHits;
            }
          break;
        }
        ++tagCount;
      }
    }
  }

private:

  bool InSources;
  bool InSource;
  std::vector<std::string> FilePaths;
  typedef cmCTestCoverageHandlerContainer::SingleFileCoverageVector
     FileLinesType;
  cmCTest* CTest;
  cmCTestCoverageHandlerContainer& Coverage;
  std::string CurFileName;

};


cmParseCoberturaCoverage::cmParseCoberturaCoverage(
    cmCTestCoverageHandlerContainer& cont,
    cmCTest* ctest)
    :Coverage(cont), CTest(ctest)
{
}

bool cmParseCoberturaCoverage::ReadCoverageXML(const char* xmlFile)
{
  cmParseCoberturaCoverage::XMLParser parser(this->CTest, this->Coverage);
  parser.ParseFile(xmlFile);
  return true;
}
