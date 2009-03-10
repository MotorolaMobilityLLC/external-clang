//===--- Diagnostic.h - C Language Family Diagnostic Handling ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the Diagnostic-related interfaces.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_DIAGNOSTIC_H
#define LLVM_CLANG_DIAGNOSTIC_H

#include "clang/Basic/SourceLocation.h"
#include <string>
#include <cassert>

namespace llvm {
  template <typename T> class SmallVectorImpl;
}

namespace clang {
  class DiagnosticClient;
  class SourceRange;
  class DiagnosticBuilder;
  class IdentifierInfo;
  
  // Import the diagnostic enums themselves.
  namespace diag {
    // Start position for diagnostics.
    enum {
      DIAG_START_LEX      =                        300,
      DIAG_START_PARSE    = DIAG_START_LEX      +  300,
      DIAG_START_AST      = DIAG_START_PARSE    +  300,
      DIAG_START_SEMA     = DIAG_START_AST      +  100,
      DIAG_START_ANALYSIS = DIAG_START_SEMA     + 1000,
      DIAG_UPPER_LIMIT    = DIAG_START_ANALYSIS +  100
    };

    class CustomDiagInfo;
    
    /// diag::kind - All of the diagnostics that can be emitted by the frontend.
    typedef unsigned kind;

    // Get typedefs for common diagnostics.
    enum {
#define DIAG(ENUM,FLAGS,DESC) ENUM,
#include "clang/Basic/DiagnosticCommonKinds.def"
      NUM_BUILTIN_COMMON_DIAGNOSTICS
#undef DIAG
    };
      
    /// Enum values that allow the client to map NOTEs, WARNINGs, and EXTENSIONs
    /// to either MAP_IGNORE (nothing), MAP_WARNING (emit a warning), MAP_ERROR
    /// (emit as an error), or MAP_DEFAULT (handle the default way).  It allows
    /// clients to map errors to MAP_ERROR/MAP_DEFAULT or MAP_FATAL (stop
    /// emitting diagnostics after this one).
    enum Mapping {
      MAP_DEFAULT = 0,     //< Do not map this diagnostic.
      MAP_IGNORE  = 1,     //< Map this diagnostic to nothing, ignore it.
      MAP_WARNING = 2,     //< Map this diagnostic to a warning.
      MAP_ERROR   = 3,     //< Map this diagnostic to an error.
      MAP_FATAL   = 4      //< Map this diagnostic to a fatal error.
    };
  }
  
/// \brief Annotates a diagnostic with some code that should be
/// inserted, removed, or replaced to fix the problem.
///
/// This kind of hint should be used when we are certain that the
/// introduction, removal, or modification of a particular (small!)
/// amount of code will correct a compilation error. The compiler
/// should also provide full recovery from such errors, such that
/// suppressing the diagnostic output can still result in successful
/// compilation.
class CodeModificationHint {
public:
  /// \brief Tokens that should be removed to correct the error.
  SourceRange RemoveRange;

  /// \brief The location at which we should insert code to correct
  /// the error.
  SourceLocation InsertionLoc;

  /// \brief The actual code to insert at the insertion location, as a
  /// string.
  std::string CodeToInsert;

  /// \brief Empty code modification hint, indicating that no code
  /// modification is known.
  CodeModificationHint() : RemoveRange(), InsertionLoc() { }

  /// \brief Create a code modification hint that inserts the given
  /// code string at a specific location.
  static CodeModificationHint CreateInsertion(SourceLocation InsertionLoc, 
                                              const std::string &Code) {
    CodeModificationHint Hint;
    Hint.InsertionLoc = InsertionLoc;
    Hint.CodeToInsert = Code;
    return Hint;
  }

  /// \brief Create a code modification hint that removes the given
  /// source range.
  static CodeModificationHint CreateRemoval(SourceRange RemoveRange) {
    CodeModificationHint Hint;
    Hint.RemoveRange = RemoveRange;
    return Hint;
  }

  /// \brief Create a code modification hint that replaces the given
  /// source range with the given code string.
  static CodeModificationHint CreateReplacement(SourceRange RemoveRange, 
                                                const std::string &Code) {
    CodeModificationHint Hint;
    Hint.RemoveRange = RemoveRange;
    Hint.InsertionLoc = RemoveRange.getBegin();
    Hint.CodeToInsert = Code;
    return Hint;
  }
};

/// \brief A hook function that will be invoked after we have
/// completed processing of the current diagnostic.
///
/// Hook functions are typically used to emit further diagnostics
/// (typically notes) that give more information about this
/// diagnostic.
struct PostDiagnosticHook {
  /// \brief The type of the hook function itself.
  ///
  /// DiagID is the ID of the diagnostic to which the hook was
  /// attached, and Cookie is a user-specified value that can be used
  /// to store extra data for the hook.
  typedef void (*HookTy)(unsigned DiagID, void *Cookie);

  PostDiagnosticHook() {}

  PostDiagnosticHook(HookTy Hook, void *Cookie) : Hook(Hook), Cookie(Cookie) {
    assert(Hook && "No hook provided!");
  }

  /// \brief The hook function.
  HookTy Hook;

  /// \brief The cookie that will be passed along to the hook function.
  void *Cookie;
};

/// Diagnostic - This concrete class is used by the front-end to report
/// problems and issues.  It massages the diagnostics (e.g. handling things like
/// "report warnings as errors" and passes them off to the DiagnosticClient for
/// reporting to the user.
class Diagnostic {
public:
  /// Level - The level of the diagnostic, after it has been through mapping.
  enum Level {
    Ignored, Note, Warning, Error, Fatal
  };
  
  enum ArgumentKind {
    ak_std_string,      // std::string
    ak_c_string,        // const char *
    ak_sint,            // int
    ak_uint,            // unsigned
    ak_identifierinfo,  // IdentifierInfo
    ak_qualtype,        // QualType
    ak_declarationname, // DeclarationName
    ak_nameddecl        // NamedDecl *
  };

private:  
  bool IgnoreAllWarnings;     // Ignore all warnings: -w
  bool WarningsAsErrors;      // Treat warnings like errors: 
  bool WarnOnExtensions;      // Enables warnings for gcc extensions: -pedantic.
  bool ErrorOnExtensions;     // Error on extensions: -pedantic-errors.
  bool SuppressSystemWarnings;// Suppress warnings in system headers.
  DiagnosticClient *Client;

  /// DiagMappings - Mapping information for diagnostics.  Mapping info is
  /// packed into four bits per diagnostic.
  unsigned char DiagMappings[diag::DIAG_UPPER_LIMIT/2];
  
  /// ErrorOccurred / FatalErrorOccurred - This is set to true when an error or
  /// fatal error is emitted, and is sticky.
  bool ErrorOccurred;
  bool FatalErrorOccurred;
  
  /// LastDiagLevel - This is the level of the last diagnostic emitted.  This is
  /// used to emit continuation diagnostics with the same level as the
  /// diagnostic that they follow.
  Diagnostic::Level LastDiagLevel;

  unsigned NumDiagnostics;    // Number of diagnostics reported
  unsigned NumErrors;         // Number of diagnostics that are errors

  /// CustomDiagInfo - Information for uniquing and looking up custom diags.
  diag::CustomDiagInfo *CustomDiagInfo;

  /// ArgToStringFn - A function pointer that converts an opaque diagnostic
  /// argument to a strings.  This takes the modifiers and argument that was
  /// present in the diagnostic.
  /// This is a hack to avoid a layering violation between libbasic and libsema.
  typedef void (*ArgToStringFnTy)(ArgumentKind Kind, intptr_t Val,
                                  const char *Modifier, unsigned ModifierLen,
                                  const char *Argument, unsigned ArgumentLen,
                                  llvm::SmallVectorImpl<char> &Output,
                                  void *Cookie);
  void *ArgToStringCookie;
  ArgToStringFnTy ArgToStringFn;
public:
  explicit Diagnostic(DiagnosticClient *client = 0);
  ~Diagnostic();
  
  //===--------------------------------------------------------------------===//
  //  Diagnostic characterization methods, used by a client to customize how
  //
  
  DiagnosticClient *getClient() { return Client; };
  const DiagnosticClient *getClient() const { return Client; };
    
  void setClient(DiagnosticClient* client) { Client = client; }

  /// setIgnoreAllWarnings - When set to true, any unmapped warnings are
  /// ignored.  If this and WarningsAsErrors are both set, then this one wins.
  void setIgnoreAllWarnings(bool Val) { IgnoreAllWarnings = Val; }
  bool getIgnoreAllWarnings() const { return IgnoreAllWarnings; }
  
  /// setWarningsAsErrors - When set to true, any warnings reported are issued
  /// as errors.
  void setWarningsAsErrors(bool Val) { WarningsAsErrors = Val; }
  bool getWarningsAsErrors() const { return WarningsAsErrors; }
  
  /// setWarnOnExtensions - When set to true, issue warnings on GCC extensions,
  /// the equivalent of GCC's -pedantic.
  void setWarnOnExtensions(bool Val) { WarnOnExtensions = Val; }
  bool getWarnOnExtensions() const { return WarnOnExtensions; }
  
  /// setErrorOnExtensions - When set to true issue errors for GCC extensions
  /// instead of warnings.  This is the equivalent to GCC's -pedantic-errors.
  void setErrorOnExtensions(bool Val) { ErrorOnExtensions = Val; }
  bool getErrorOnExtensions() const { return ErrorOnExtensions; }

  /// setSuppressSystemWarnings - When set to true mask warnings that
  /// come from system headers.
  void setSuppressSystemWarnings(bool Val) { SuppressSystemWarnings = Val; }
  bool getSuppressSystemWarnings() const { return SuppressSystemWarnings; }

  /// setDiagnosticMapping - This allows the client to specify that certain
  /// warnings are ignored.  Only WARNINGs and EXTENSIONs can be mapped.
  void setDiagnosticMapping(diag::kind Diag, diag::Mapping Map) {
    assert(Diag < diag::DIAG_UPPER_LIMIT &&
           "Can only map builtin diagnostics");
    assert((isBuiltinWarningOrExtension(Diag) || Map == diag::MAP_FATAL) &&
           "Cannot map errors!");
    unsigned char &Slot = DiagMappings[Diag/2];
    unsigned Bits = (Diag & 1)*4;
    Slot &= ~(7 << Bits);
    Slot |= Map << Bits;
  }

  /// getDiagnosticMapping - Return the mapping currently set for the specified
  /// diagnostic.
  diag::Mapping getDiagnosticMapping(diag::kind Diag) const {
    return (diag::Mapping)((DiagMappings[Diag/2] >> (Diag & 1)*4) & 7);
  }
  
  bool hasErrorOccurred() const { return ErrorOccurred; }
  bool hasFatalErrorOccurred() const { return FatalErrorOccurred; }

  unsigned getNumErrors() const { return NumErrors; }
  unsigned getNumDiagnostics() const { return NumDiagnostics; }
  
  /// getCustomDiagID - Return an ID for a diagnostic with the specified message
  /// and level.  If this is the first request for this diagnosic, it is
  /// registered and created, otherwise the existing ID is returned.
  unsigned getCustomDiagID(Level L, const char *Message);
  
  
  /// ConvertArgToString - This method converts a diagnostic argument (as an
  /// intptr_t) into the string that represents it.
  void ConvertArgToString(ArgumentKind Kind, intptr_t Val,
                          const char *Modifier, unsigned ModLen,
                          const char *Argument, unsigned ArgLen,
                          llvm::SmallVectorImpl<char> &Output) const {
    ArgToStringFn(Kind, Val, Modifier, ModLen, Argument, ArgLen, Output,
                  ArgToStringCookie);
  }
  
  void SetArgToStringFn(ArgToStringFnTy Fn, void *Cookie) {
    ArgToStringFn = Fn;
    ArgToStringCookie = Cookie;
  }
  
  //===--------------------------------------------------------------------===//
  // Diagnostic classification and reporting interfaces.
  //

  /// getDescription - Given a diagnostic ID, return a description of the
  /// issue.
  const char *getDescription(unsigned DiagID) const;
  
  /// isNoteWarningOrExtension - Return true if the unmapped diagnostic
  /// level of the specified diagnostic ID is a Warning or Extension.
  /// This only works on builtin diagnostics, not custom ones, and is not legal to
  /// call on NOTEs.
  static bool isBuiltinWarningOrExtension(unsigned DiagID);

  /// \brief Determine whether the given built-in diagnostic ID is a
  /// Note.
  static bool isBuiltinNote(unsigned DiagID);

  /// getDiagnosticLevel - Based on the way the client configured the Diagnostic
  /// object, classify the specified diagnostic ID into a Level, consumable by
  /// the DiagnosticClient.
  Level getDiagnosticLevel(unsigned DiagID) const;  
  
  /// Report - Issue the message to the client.  @c DiagID is a member of the
  /// @c diag::kind enum.  This actually returns aninstance of DiagnosticBuilder
  /// which emits the diagnostics (through @c ProcessDiag) when it is destroyed.
  /// @c Pos represents the source location associated with the diagnostic,
  /// which can be an invalid location if no position information is available.
  inline DiagnosticBuilder Report(FullSourceLoc Pos, unsigned DiagID);
  
private:
  /// getDiagnosticLevel - This is an internal implementation helper used when
  /// DiagClass is already known.
  Level getDiagnosticLevel(unsigned DiagID, unsigned DiagClass) const;

  // This is private state used by DiagnosticBuilder.  We put it here instead of
  // in DiagnosticBuilder in order to keep DiagnosticBuilder a small lightweight
  // object.  This implementation choice means that we can only have one
  // diagnostic "in flight" at a time, but this seems to be a reasonable
  // tradeoff to keep these objects small.  Assertions verify that only one
  // diagnostic is in flight at a time.
  friend class DiagnosticBuilder;
  friend class DiagnosticInfo;

  /// CurDiagLoc - This is the location of the current diagnostic that is in
  /// flight.
  FullSourceLoc CurDiagLoc;
  
  /// CurDiagID - This is the ID of the current diagnostic that is in flight.
  /// This is set to ~0U when there is no diagnostic in flight.
  unsigned CurDiagID;

  enum {
    /// MaxArguments - The maximum number of arguments we can hold. We currently
    /// only support up to 10 arguments (%0-%9).  A single diagnostic with more
    /// than that almost certainly has to be simplified anyway.
    MaxArguments = 10
  };
  
  /// NumDiagArgs - This contains the number of entries in Arguments.
  signed char NumDiagArgs;
  /// NumRanges - This is the number of ranges in the DiagRanges array.
  unsigned char NumDiagRanges;
  /// \brief The number of code modifications hints in the
  /// CodeModificationHints array.
  unsigned char NumCodeModificationHints;
  /// \brief The number of post-diagnostic hooks in the
  /// PostDiagnosticHooks array.
  unsigned char NumPostDiagnosticHooks;

  /// DiagArgumentsKind - This is an array of ArgumentKind::ArgumentKind enum
  /// values, with one for each argument.  This specifies whether the argument
  /// is in DiagArgumentsStr or in DiagArguments.
  unsigned char DiagArgumentsKind[MaxArguments];
  
  /// DiagArgumentsStr - This holds the values of each string argument for the
  /// current diagnostic.  This value is only used when the corresponding
  /// ArgumentKind is ak_std_string.
  std::string DiagArgumentsStr[MaxArguments];

  /// DiagArgumentsVal - The values for the various substitution positions. This
  /// is used when the argument is not an std::string.  The specific value is
  /// mangled into an intptr_t and the intepretation depends on exactly what
  /// sort of argument kind it is.
  intptr_t DiagArgumentsVal[MaxArguments];
  
  /// DiagRanges - The list of ranges added to this diagnostic.  It currently
  /// only support 10 ranges, could easily be extended if needed.
  const SourceRange *DiagRanges[10];
  
  enum { MaxCodeModificationHints = 3 };

  /// CodeModificationHints - If valid, provides a hint with some code
  /// to insert, remove, or modify at a particular position.
  CodeModificationHint CodeModificationHints[MaxCodeModificationHints];

  enum { MaxPostDiagnosticHooks = 10 };
  
  /// \brief Functions that will be invoked after the diagnostic has
  /// been emitted.
  PostDiagnosticHook PostDiagnosticHooks[MaxPostDiagnosticHooks];

  /// \brief Whether we're already within a post-diagnostic hook.
  bool InPostDiagnosticHook;

  /// ProcessDiag - This is the method used to report a diagnostic that is
  /// finally fully formed.
  void ProcessDiag();
};

//===----------------------------------------------------------------------===//
// DiagnosticBuilder
//===----------------------------------------------------------------------===//

/// DiagnosticBuilder - This is a little helper class used to produce
/// diagnostics.  This is constructed by the Diagnostic::Report method, and
/// allows insertion of extra information (arguments and source ranges) into the
/// currently "in flight" diagnostic.  When the temporary for the builder is
/// destroyed, the diagnostic is issued.
///
/// Note that many of these will be created as temporary objects (many call
/// sites), so we want them to be small and we never want their address taken.
/// This ensures that compilers with somewhat reasonable optimizers will promote
/// the common fields to registers, eliminating increments of the NumArgs field,
/// for example.
class DiagnosticBuilder {
  mutable Diagnostic *DiagObj;
  mutable unsigned NumArgs, NumRanges, NumCodeModificationHints, 
                   NumPostDiagnosticHooks;
  
  void operator=(const DiagnosticBuilder&); // DO NOT IMPLEMENT
  friend class Diagnostic;
  explicit DiagnosticBuilder(Diagnostic *diagObj)
    : DiagObj(diagObj), NumArgs(0), NumRanges(0), 
      NumCodeModificationHints(0), NumPostDiagnosticHooks(0) {}

public:  
  /// Copy constructor.  When copied, this "takes" the diagnostic info from the
  /// input and neuters it.
  DiagnosticBuilder(const DiagnosticBuilder &D) {
    DiagObj = D.DiagObj;
    D.DiagObj = 0;
  }
  
  /// Destructor - The dtor emits the diagnostic.
  ~DiagnosticBuilder() {
    // If DiagObj is null, then its soul was stolen by the copy ctor.
    if (DiagObj == 0) return;

    // When destroyed, the ~DiagnosticBuilder sets the final argument count into
    // the Diagnostic object.
    DiagObj->NumDiagArgs = NumArgs;
    DiagObj->NumDiagRanges = NumRanges;
    DiagObj->NumCodeModificationHints = NumCodeModificationHints;
    DiagObj->NumPostDiagnosticHooks = NumPostDiagnosticHooks;

    // Process the diagnostic, sending the accumulated information to the
    // DiagnosticClient.
    DiagObj->ProcessDiag();

    // This diagnostic is no longer in flight.
    DiagObj->CurDiagID = ~0U;
  }
  
  /// Operator bool: conversion of DiagnosticBuilder to bool always returns
  /// true.  This allows is to be used in boolean error contexts like:
  /// return Diag(...);
  operator bool() const { return true; }

  void AddString(const std::string &S) const {
    assert(NumArgs < Diagnostic::MaxArguments &&
           "Too many arguments to diagnostic!");
    DiagObj->DiagArgumentsKind[NumArgs] = Diagnostic::ak_std_string;
    DiagObj->DiagArgumentsStr[NumArgs++] = S;
  }
  
  void AddTaggedVal(intptr_t V, Diagnostic::ArgumentKind Kind) const {
    assert(NumArgs < Diagnostic::MaxArguments &&
           "Too many arguments to diagnostic!");
    DiagObj->DiagArgumentsKind[NumArgs] = Kind;
    DiagObj->DiagArgumentsVal[NumArgs++] = V;
  }
  
  void AddSourceRange(const SourceRange &R) const {
    assert(NumRanges < 
           sizeof(DiagObj->DiagRanges)/sizeof(DiagObj->DiagRanges[0]) &&
           "Too many arguments to diagnostic!");
    DiagObj->DiagRanges[NumRanges++] = &R;
  }    

  void AddCodeModificationHint(const CodeModificationHint &Hint) const {
    assert(NumCodeModificationHints < Diagnostic::MaxCodeModificationHints &&
           "Too many code modification hints!");
    DiagObj->CodeModificationHints[NumCodeModificationHints++] = Hint;
  }

  void AddPostDiagnosticHook(const PostDiagnosticHook &Hook) const {
    assert(!DiagObj->InPostDiagnosticHook &&
           "Can't add a post-diagnostic hook to a diagnostic inside "
           "a post-diagnostic hook");
    assert(NumPostDiagnosticHooks < Diagnostic::MaxPostDiagnosticHooks &&
           "Too many post-diagnostic hooks");
    DiagObj->PostDiagnosticHooks[NumPostDiagnosticHooks++] = Hook;
  }
};

inline const DiagnosticBuilder &operator<<(const DiagnosticBuilder &DB,
                                           const std::string &S) {
  DB.AddString(S);
  return DB;
}

inline const DiagnosticBuilder &operator<<(const DiagnosticBuilder &DB,
                                           const char *Str) {
  DB.AddTaggedVal(reinterpret_cast<intptr_t>(Str),
                  Diagnostic::ak_c_string);
  return DB;
}

inline const DiagnosticBuilder &operator<<(const DiagnosticBuilder &DB, int I) {
  DB.AddTaggedVal(I, Diagnostic::ak_sint);
  return DB;
}

inline const DiagnosticBuilder &operator<<(const DiagnosticBuilder &DB,bool I) {
  DB.AddTaggedVal(I, Diagnostic::ak_sint);
  return DB;
}

inline const DiagnosticBuilder &operator<<(const DiagnosticBuilder &DB,
                                           unsigned I) {
  DB.AddTaggedVal(I, Diagnostic::ak_uint);
  return DB;
}

inline const DiagnosticBuilder &operator<<(const DiagnosticBuilder &DB,
                                           const IdentifierInfo *II) {
  DB.AddTaggedVal(reinterpret_cast<intptr_t>(II),
                  Diagnostic::ak_identifierinfo);
  return DB;
}
  
inline const DiagnosticBuilder &operator<<(const DiagnosticBuilder &DB,
                                           const SourceRange &R) {
  DB.AddSourceRange(R);
  return DB;
}

inline const DiagnosticBuilder &operator<<(const DiagnosticBuilder &DB,
                                           const CodeModificationHint &Hint) {
  DB.AddCodeModificationHint(Hint);
  return DB;
}

inline const DiagnosticBuilder &operator<<(const DiagnosticBuilder &DB,
                                           const PostDiagnosticHook &Hook) {
  DB.AddPostDiagnosticHook(Hook);
  return DB;
}
  

/// Report - Issue the message to the client.  DiagID is a member of the
/// diag::kind enum.  This actually returns a new instance of DiagnosticBuilder
/// which emits the diagnostics (through ProcessDiag) when it is destroyed.
inline DiagnosticBuilder Diagnostic::Report(FullSourceLoc Loc, unsigned DiagID){
  assert(CurDiagID == ~0U && "Multiple diagnostics in flight at once!");
  CurDiagLoc = Loc;
  CurDiagID = DiagID;
  return DiagnosticBuilder(this);
}

//===----------------------------------------------------------------------===//
// DiagnosticInfo
//===----------------------------------------------------------------------===//
  
/// DiagnosticInfo - This is a little helper class (which is basically a smart
/// pointer that forward info from Diagnostic) that allows clients to enquire
/// about the currently in-flight diagnostic.
class DiagnosticInfo {
  const Diagnostic *DiagObj;
public:
  explicit DiagnosticInfo(const Diagnostic *DO) : DiagObj(DO) {}
  
  const Diagnostic *getDiags() const { return DiagObj; }
  unsigned getID() const { return DiagObj->CurDiagID; }
  const FullSourceLoc &getLocation() const { return DiagObj->CurDiagLoc; }
  
  unsigned getNumArgs() const { return DiagObj->NumDiagArgs; }
  
  /// getArgKind - Return the kind of the specified index.  Based on the kind
  /// of argument, the accessors below can be used to get the value.
  Diagnostic::ArgumentKind getArgKind(unsigned Idx) const {
    assert(Idx < getNumArgs() && "Argument index out of range!");
    return (Diagnostic::ArgumentKind)DiagObj->DiagArgumentsKind[Idx];
  }
  
  /// getArgStdStr - Return the provided argument string specified by Idx.
  const std::string &getArgStdStr(unsigned Idx) const {
    assert(getArgKind(Idx) == Diagnostic::ak_std_string &&
           "invalid argument accessor!");
    return DiagObj->DiagArgumentsStr[Idx];
  }
  
  /// getArgCStr - Return the specified C string argument.
  const char *getArgCStr(unsigned Idx) const {
    assert(getArgKind(Idx) == Diagnostic::ak_c_string &&
           "invalid argument accessor!");
    return reinterpret_cast<const char*>(DiagObj->DiagArgumentsVal[Idx]);
  }
  
  /// getArgSInt - Return the specified signed integer argument.
  int getArgSInt(unsigned Idx) const {
    assert(getArgKind(Idx) == Diagnostic::ak_sint &&
           "invalid argument accessor!");
    return (int)DiagObj->DiagArgumentsVal[Idx];
  }
  
  /// getArgUInt - Return the specified unsigned integer argument.
  unsigned getArgUInt(unsigned Idx) const {
    assert(getArgKind(Idx) == Diagnostic::ak_uint &&
           "invalid argument accessor!");
    return (unsigned)DiagObj->DiagArgumentsVal[Idx];
  }
  
  /// getArgIdentifier - Return the specified IdentifierInfo argument.
  const IdentifierInfo *getArgIdentifier(unsigned Idx) const {
    assert(getArgKind(Idx) == Diagnostic::ak_identifierinfo &&
           "invalid argument accessor!");
    return reinterpret_cast<IdentifierInfo*>(DiagObj->DiagArgumentsVal[Idx]);
  }
  
  /// getRawArg - Return the specified non-string argument in an opaque form.
  intptr_t getRawArg(unsigned Idx) const {
    assert(getArgKind(Idx) != Diagnostic::ak_std_string &&
           "invalid argument accessor!");
    return DiagObj->DiagArgumentsVal[Idx];
  }
  
  
  /// getNumRanges - Return the number of source ranges associated with this
  /// diagnostic.
  unsigned getNumRanges() const {
    return DiagObj->NumDiagRanges;
  }
  
  const SourceRange &getRange(unsigned Idx) const {
    assert(Idx < DiagObj->NumDiagRanges && "Invalid diagnostic range index!");
    return *DiagObj->DiagRanges[Idx];
  }
  
  unsigned getNumCodeModificationHints() const {
    return DiagObj->NumCodeModificationHints;
  }

  const CodeModificationHint &getCodeModificationHint(unsigned Idx) const {
    return DiagObj->CodeModificationHints[Idx];
  }

  const CodeModificationHint *getCodeModificationHints() const {
    return DiagObj->NumCodeModificationHints? 
             &DiagObj->CodeModificationHints[0] : 0;
  }

  /// FormatDiagnostic - Format this diagnostic into a string, substituting the
  /// formal arguments into the %0 slots.  The result is appended onto the Str
  /// array.
  void FormatDiagnostic(llvm::SmallVectorImpl<char> &OutStr) const;
};
  
/// DiagnosticClient - This is an abstract interface implemented by clients of
/// the front-end, which formats and prints fully processed diagnostics.
class DiagnosticClient {
public:
  virtual ~DiagnosticClient();
  
  /// IncludeInDiagnosticCounts - This method (whose default implementation
  ///  returns true) indicates whether the diagnostics handled by this
  ///  DiagnosticClient should be included in the number of diagnostics
  ///  reported by Diagnostic.
  virtual bool IncludeInDiagnosticCounts() const;

  /// HandleDiagnostic - Handle this diagnostic, reporting it to the user or
  /// capturing it to a log as needed.
  virtual void HandleDiagnostic(Diagnostic::Level DiagLevel,
                                const DiagnosticInfo &Info) = 0;
};

}  // end namespace clang

#endif
