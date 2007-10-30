//===--- SourceManager.h - Track and cache source files ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by Chris Lattner and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the SourceManager interface.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_SOURCEMANAGER_H
#define LLVM_CLANG_SOURCEMANAGER_H

#include "clang/Basic/SourceLocation.h"
#include "llvm/Bitcode/SerializationFwd.h"
#include <vector>
#include <set>
#include <list>
#include <cassert>

namespace llvm {
class MemoryBuffer;
}
  
namespace clang {
  
class SourceManager;
class FileEntry;
class IdentifierTokenInfo;

/// SrcMgr - Private classes that are part of the SourceManager implementation.
///
namespace SrcMgr {
  /// ContentCache - Once instance of this struct is kept for every file
  ///  loaded or used.  This object owns the MemoryBuffer object.
  struct ContentCache {
    /// Reference to the file entry.  This reference does not own
    /// the FileEntry object.  It is possible for this to be NULL if
    /// the ContentCache encapsulates an imaginary text buffer.
    const FileEntry* Entry;
    
    /// Buffer - The actual buffer containing the characters from the input
    /// file.  This is owned by the FileInfo object.
    const llvm::MemoryBuffer* Buffer;
    
    /// SourceLineCache - A new[]'d array of offsets for each source line.  This
    /// is lazily computed.  This is owned by the FileInfo object.
    unsigned* SourceLineCache;
    
    /// NumLines - The number of lines in this FileInfo.  This is only valid if
    /// SourceLineCache is non-null.
    unsigned NumLines;
        
    ContentCache(const FileEntry* e = NULL)
    : Entry(e), Buffer(NULL), SourceLineCache(NULL), NumLines(0) {}

    ~ContentCache();
  };  

  /// FileIDInfo - Information about a FileID, basically just the logical file
  /// that it represents and include stack information.  A File SourceLocation
  /// is a byte offset from the start of this.
  ///
  /// FileID's are used to compute the location of a character in memory as well
  /// as the logical source location, which can be differ from the physical
  /// location.  It is different when #line's are active or when macros have
  /// been expanded.
  ///
  /// Each FileID has include stack information, indicating where it came from.
  /// For the primary translation unit, it comes from SourceLocation() aka 0.
  /// This information encodes the #include chain that a token was instantiated
  /// from.
  ///
  /// FileIDInfos contain a "InfoRec *", describing the source file, and a Chunk
  /// number, which allows a SourceLocation to index into very large files
  /// (those which there are not enough FilePosBits to address).
  ///
  struct FileIDInfo {
  private:
    /// IncludeLoc - The location of the #include that brought in this file.
    /// This SourceLocation object has an invalid SLOC for the main file.
    SourceLocation IncludeLoc;
    
    /// ChunkNo - Really large buffers are broken up into chunks that are
    /// each (1 << SourceLocation::FilePosBits) in size.  This specifies the
    /// chunk number of this FileID.
    unsigned ChunkNo;
    
    /// Content - Information about the source buffer itself.
    const ContentCache* Content;
    
  public:
    /// get - Return a FileIDInfo object.
    static FileIDInfo get(SourceLocation IL, unsigned CN, 
                          const ContentCache *Con) {
      FileIDInfo X;
      X.IncludeLoc = IL;
      X.ChunkNo = CN;
      X.Content = Con;
      return X;
    }
    
    SourceLocation getIncludeLoc() const { return IncludeLoc; }
    unsigned getChunkNo() const { return ChunkNo; }
    const ContentCache* getContentCache() const { return Content; }
  };
  
  /// MacroIDInfo - Macro SourceLocations refer to these records by their ID.
  /// Each MacroIDInfo encodes the Instantiation location - where the macro was
  /// instantiated, and the PhysicalLoc - where the actual character data for
  /// the token came from.  An actual macro SourceLocation stores deltas from
  /// these positions.
  class MacroIDInfo {
    SourceLocation InstantiationLoc, PhysicalLoc;
  public:
    SourceLocation getInstantiationLoc() const { return InstantiationLoc; }
    SourceLocation getPhysicalLoc() const { return PhysicalLoc; }
    
    /// get - Return a MacroID for a macro expansion.  IL specifies
    /// the instantiation location, and PL specifies the physical location
    /// (where the characters from the token come from).  Both IL and PL refer
    /// to normal File SLocs.
    static MacroIDInfo get(SourceLocation IL, SourceLocation PL) {
      MacroIDInfo X;
      X.InstantiationLoc = IL;
      X.PhysicalLoc = PL;
      return X;
    }
  };
}  // end SrcMgr namespace.
} // end clang namespace

namespace std {
template <> struct less<clang::SrcMgr::ContentCache> {
  inline bool operator()(const clang::SrcMgr::ContentCache& L,
                         const clang::SrcMgr::ContentCache& R) const {
    return L.Entry < R.Entry;
  }
};
} // end std namespace

namespace clang {
  
/// SourceManager - This file handles loading and caching of source files into
/// memory.  This object owns the MemoryBuffer objects for all of the loaded
/// files and assigns unique FileID's for each unique #include chain.
///
/// The SourceManager can be queried for information about SourceLocation
/// objects, turning them into either physical or logical locations.  Physical
/// locations represent where the bytes corresponding to a token came from and
/// logical locations represent where the location is in the user's view.  In
/// the case of a macro expansion, for example, the physical location indicates
/// where the expanded token came from and the logical location specifies where
/// it was expanded.  Logical locations are also influenced by #line directives,
/// etc.
class SourceManager {
  /// FileInfos - Memoized information about all of the files tracked by this
  /// SourceManager.
  std::set<SrcMgr::ContentCache> FileInfos;
  
  /// MemBufferInfos - Information about various memory buffers that we have
  /// read in.  This is a list, instead of a vector, because we need pointers to
  /// the FileInfo objects to be stable.
  std::list<SrcMgr::ContentCache> MemBufferInfos;
  
  /// FileIDs - Information about each FileID.  FileID #0 is not valid, so all
  /// entries are off by one.
  std::vector<SrcMgr::FileIDInfo> FileIDs;
  
  /// MacroIDs - Information about each MacroID.
  std::vector<SrcMgr::MacroIDInfo> MacroIDs;
  
  /// LastLineNo - These ivars serve as a cache used in the getLineNumber
  /// method which is used to speedup getLineNumber calls to nearby locations.
  unsigned LastLineNoFileIDQuery;
  SrcMgr::ContentCache *LastLineNoContentCache;
  unsigned LastLineNoFilePos;
  unsigned LastLineNoResult;
  
public:
  SourceManager() : LastLineNoFileIDQuery(~0U) {}
  ~SourceManager() {}
  
  void clearIDTables() {
    FileIDs.clear();
    MacroIDs.clear();
    LastLineNoFileIDQuery = ~0U;
    LastLineNoContentCache = 0;
  }
  
  /// createFileID - Create a new FileID that represents the specified file
  /// being #included from the specified IncludePosition.  This returns 0 on
  /// error and translates NULL into standard input.
  unsigned createFileID(const FileEntry *SourceFile, SourceLocation IncludePos){
    const SrcMgr::ContentCache *IR = getContentCache(SourceFile);
    if (IR == 0) return 0;    // Error opening file?
    return createFileID(IR, IncludePos);
  }
  
  /// createFileIDForMemBuffer - Create a new FileID that represents the
  /// specified memory buffer.  This does no caching of the buffer and takes
  /// ownership of the MemoryBuffer, so only pass a MemoryBuffer to this once.
  unsigned createFileIDForMemBuffer(const llvm::MemoryBuffer *Buffer) {
    return createFileID(createMemBufferContentCache(Buffer), SourceLocation());
  }
  
  /// getInstantiationLoc - Return a new SourceLocation that encodes the fact
  /// that a token at Loc should actually be referenced from InstantiationLoc.
  SourceLocation getInstantiationLoc(SourceLocation Loc,
                                     SourceLocation InstantiationLoc);
  
  /// getBuffer - Return the buffer for the specified FileID.
  ///
  const llvm::MemoryBuffer *getBuffer(unsigned FileID) const {
    return getContentCache(FileID)->Buffer;
  }
  
  /// getBufferData - Return a pointer to the start and end of the character
  /// data for the specified FileID.
  std::pair<const char*, const char*> getBufferData(unsigned FileID) const;
  
  /// getIncludeLoc - Return the location of the #include for the specified
  /// SourceLocation.  If this is a macro expansion, this transparently figures
  /// out which file includes the file being expanded into.
  SourceLocation getIncludeLoc(SourceLocation ID) const {
    return getFIDInfo(getLogicalLoc(ID).getFileID())->getIncludeLoc();
  }
  
  /// getCharacterData - Return a pointer to the start of the specified location
  /// in the appropriate MemoryBuffer.
  const char *getCharacterData(SourceLocation SL) const;
  
  /// getColumnNumber - Return the column # for the specified file position.
  /// This is significantly cheaper to compute than the line number.  This
  /// returns zero if the column number isn't known.  This may only be called on
  /// a file sloc, so you must choose a physical or logical location before
  /// calling this method.
  unsigned getColumnNumber(SourceLocation Loc) const;
  
  unsigned getPhysicalColumnNumber(SourceLocation Loc) const {
    return getColumnNumber(getPhysicalLoc(Loc));
  }
  unsigned getLogicalColumnNumber(SourceLocation Loc) const {
    return getColumnNumber(getLogicalLoc(Loc));
  }
  
  
  /// getLineNumber - Given a SourceLocation, return the physical line number
  /// for the position indicated.  This requires building and caching a table of
  /// line offsets for the MemoryBuffer, so this is not cheap: use only when
  /// about to emit a diagnostic.
  unsigned getLineNumber(SourceLocation Loc);

  unsigned getLogicalLineNumber(SourceLocation Loc) {
    return getLineNumber(getLogicalLoc(Loc));
  }
  unsigned getPhysicalLineNumber(SourceLocation Loc) {
    return getLineNumber(getPhysicalLoc(Loc));
  }
  
  /// getSourceName - This method returns the name of the file or buffer that
  /// the SourceLocation specifies.  This can be modified with #line directives,
  /// etc.
  const char *getSourceName(SourceLocation Loc) const;

  /// Given a SourceLocation object, return the logical location referenced by
  /// the ID.  This logical location is subject to #line directives, etc.
  SourceLocation getLogicalLoc(SourceLocation Loc) const {
    // File locations are both physical and logical.
    if (Loc.isFileID()) return Loc;

    SourceLocation ILoc = MacroIDs[Loc.getMacroID()].getInstantiationLoc();
    return ILoc.getFileLocWithOffset(Loc.getMacroLogOffs());
  }
  
  /// getPhysicalLoc - Given a SourceLocation object, return the physical
  /// location referenced by the ID.
  SourceLocation getPhysicalLoc(SourceLocation Loc) const {
    // File locations are both physical and logical.
    if (Loc.isFileID()) return Loc;
    
    SourceLocation PLoc = MacroIDs[Loc.getMacroID()].getPhysicalLoc();
    return PLoc.getFileLocWithOffset(Loc.getMacroPhysOffs());
  }

  /// getContentCacheForLoc - Return the ContentCache for the physloc of the 
  /// specified SourceLocation, if one exists.
  const SrcMgr::ContentCache* getContentCacheForLoc(SourceLocation Loc) const {
    Loc = getPhysicalLoc(Loc);
    unsigned FileID = Loc.getFileID();
    assert(FileID-1 < FileIDs.size() && "Invalid FileID!");
    return FileIDs[FileID-1].getContentCache();
  }
  
  /// getFileEntryForLoc - Return the FileEntry record for the physloc of the
  ///  specified SourceLocation, if one exists.
  const FileEntry* getFileEntryForLoc(SourceLocation Loc) const {
    return getContentCacheForLoc(Loc)->Entry;
  }
  
  /// getDecomposedFileLoc - Decompose the specified file location into a raw
  /// FileID + Offset pair.  The first element is the FileID, the second is the
  /// offset from the start of the buffer of the location.
  std::pair<unsigned, unsigned> getDecomposedFileLoc(SourceLocation Loc) const {
    assert(Loc.isFileID() && "Isn't a File SourceLocation");
    
    // TODO: Add a flag "is first chunk" to SLOC.
    const SrcMgr::FileIDInfo *FIDInfo = getFIDInfo(Loc.getFileID());
      
    // If this file has been split up into chunks, factor in the chunk number
    // that the FileID references.
    unsigned ChunkNo = FIDInfo->getChunkNo();
    unsigned Offset = Loc.getRawFilePos();
    Offset += (ChunkNo << SourceLocation::FilePosBits);
    
    return std::pair<unsigned,unsigned>(Loc.getFileID()-ChunkNo, Offset);
  }
  
  /// PrintStats - Print statistics to stderr.
  ///
  void PrintStats() const;

private:
  /// createFileID - Create a new fileID for the specified ContentCache and
  ///  include position.  This works regardless of whether the ContentCache
  ///  corresponds to a file or some other input source.
  unsigned createFileID(const SrcMgr::ContentCache* File,
                        SourceLocation IncludePos);
    
  /// getContentCache - Create or return a cached ContentCache for the specified
  ///  file.  This returns null on failure.
  const SrcMgr::ContentCache* getContentCache(const FileEntry* SourceFile);
  
  /// createMemBufferContentCache - Create a new ContentCache for the specified
  ///  memory buffer.
  const SrcMgr::ContentCache* 
  createMemBufferContentCache(const llvm::MemoryBuffer* Buf);

  const SrcMgr::FileIDInfo* getFIDInfo(unsigned FileID) const {
    assert(FileID-1 < FileIDs.size() && "Invalid FileID!");
    return &FileIDs[FileID-1];
  }
  
  const SrcMgr::ContentCache *getContentCache(unsigned FileID) const {
    return getContentCache(getFIDInfo(FileID));
  }
  
  /// Return the ContentCache structure for the specified FileID.  
  ///  This is always the physical reference for the ID.
  const SrcMgr::ContentCache*
  getContentCache(const SrcMgr::FileIDInfo* FIDInfo) const {
    return FIDInfo->getContentCache();
  }  
  
  /// getFullFilePos - This (efficient) method returns the offset from the start
  /// of the file that the specified physical SourceLocation represents.  This
  /// returns the location of the physical character data, not the logical file
  /// position.
  unsigned getFullFilePos(SourceLocation PhysLoc) const {
    return getDecomposedFileLoc(PhysLoc).second;
  }
};


}  // end namespace clang

#endif
