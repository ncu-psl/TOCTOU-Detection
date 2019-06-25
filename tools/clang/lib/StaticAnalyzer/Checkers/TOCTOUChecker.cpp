#include "ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include <set>
#include <iostream>
#include <string>
#include <memory>

using namespace clang;
using namespace ento;

using std::string;
using std::vector;
using std::cout;
using std::endl;



class MyString : public string{


public:
    MyString():string(){}
    MyString(const string &s):string(s){}
    MyString(const char* s):string (s){}
    void Profile (llvm::FoldingSetNodeID &ID) const{
        ID.AddString((string)(*this));
    }
};

class VarState{
private:
    enum Status {UnChecked, Checked, Used} S;
    string CF, UF;
public:
    VarState(unsigned InS, string InCF="", string InUF=""): S( (Status) InS), CF(InCF), UF(InUF){}
    bool isChecked()const {return S == Checked;}
    bool isUsed()const {return S == Used;}
    static unsigned getUnChecked() {return (unsigned) UnChecked;}
    static unsigned getChecked() {return (unsigned) Checked;}
    static unsigned getUsed() {return (unsigned) Used;}
    string getCheckFunc() const{return CF;}
    string getUseFunc() const{return UF;}
    bool operator==(const VarState &X) const{
        return S == X.S && CF == X.CF && UF == X.UF;    
    }
    void Profile (llvm::FoldingSetNodeID &ID) const{
        ID.AddInteger(S);
        ID.AddString(CF);
        ID.AddString(UF);
    }

}; 




class TOCTOUChecker : public Checker<check::PreCall> {

    mutable std::set< const IdentifierInfo* > CheckSet, UseSet, AllSet;

    OwningPtr<BugType> TOCTOUBugType;

    void initIdentifierInfo(ASTContext &Ctx) const;
    void reportTOCTOU(const CallEvent &Call, CheckerContext &C, string checkFunc, string useFunc) const;

public:
    TOCTOUChecker();
    void checkPreCall(const CallEvent &Call, CheckerContext &C) const;
    void printSValKind(const SVal sv) const;
};

REGISTER_MAP_WITH_PROGRAMSTATE(CFS, SymbolRef, VarState)
REGISTER_MAP_WITH_PROGRAMSTATE(SCFS, MyString, VarState)




TOCTOUChecker::TOCTOUChecker(){
  
    TOCTOUBugType.reset( new BugType("TOCTOU", "Race Condition") );

} 

void TOCTOUChecker::initIdentifierInfo(ASTContext &Ctx) const{
    if (!CheckSet.empty())
        return;

    //initialize Set
    string CheckSetList[] = {"access", "stat", "fopen", "creat", "mknod", "link", "symlink", "mkdir", "unlink", "rmdir", "rename", "execve", "chmod", "chown", "truncate", "utime", "chdir", "chroot", "pivot_root", "mount"};
    string UseSetList[] = {"wow","creat", "mknod", "mkdir", "rename", "link", "symlink", "open", "execve", "chdir", "chroot", "pivot_root", "mount", "chmod", "chown", "truncate", "utime"};
    string AllSetList[] = {"wow","access", "stat", "fopen", "creat", "mknod", "link", "symlink", "mkdir", "unlink", "rmdir", "rename", "execve", "chmod", "chown", "truncate", "utime", "chdir", "chroot", "pivot_root", "mount"};

    for(size_t i=0, end=sizeof(CheckSetList)/sizeof(string); i<end; i++){
        CheckSet.insert(&Ctx.Idents.get(CheckSetList[i]));
    }
    for(size_t i=0, end=sizeof(UseSetList)/sizeof(string); i<end; i++){
        UseSet.insert(&Ctx.Idents.get(UseSetList[i]));
    }
    for(size_t i=0, end=sizeof(AllSetList)/sizeof(string); i<end; i++){
        AllSet.insert(&Ctx.Idents.get(AllSetList[i]));
    }

}


void TOCTOUChecker::checkPreCall(const CallEvent &Call, CheckerContext &C) const{
    
    initIdentifierInfo(C.getASTContext());

    if(!Call.isGlobalCFunction())
        return;

   
    

    //CheckSet or UseSet  
    if(CheckSet.find(Call.getCalleeIdentifier()) != CheckSet.end() ){
        
            SVal sv = Call.getArgSVal(0);
            
            //if string
            if(!sv.getAsSymbol()){
                MyString param = MyString(sv.getAsRegion()->getString());

                
                ProgramStateRef State = C.getState();
                const VarState *S = State->get<SCFS>(param);
                if(S && S->isChecked()){
                    //no-op
                     ((void)0); 
                }
                else{

                    string funcName = Call.getCalleeIdentifier()->getName();
                    State = State->set<SCFS>(param, VarState(VarState::getChecked(), funcName));
                    C.addTransition(State);

                    return;
                }   
            }
            //if symbol
            else{
                SymbolRef param = sv.getAsSymbol();


                ProgramStateRef State = C.getState();
                const VarState *S = State->get<CFS>(param);
                if(S && S->isChecked()){
                    //no-op
                    ((void)0); 
                }
                else{

                    string funcName = Call.getCalleeIdentifier()->getName();
                    State = State->set<CFS>(param, VarState(VarState::getChecked(), funcName));
                    C.addTransition(State);

                    return;
                }
            } 


    }

    if(UseSet.find(Call.getCalleeIdentifier()) != UseSet.end() ){
        
            SVal sv = Call.getArgSVal(0);

            //if string
            if(!sv.getAsSymbol()){
                MyString param = MyString(sv.getAsRegion()->getString());

                
                ProgramStateRef State = C.getState();
                const VarState *S = State->get<SCFS>(param);
                if(S && S->isChecked()){
                    
                    string checkFunc = S->getCheckFunc();
                    string useFunc = Call.getCalleeIdentifier()->getName();

                    reportTOCTOU(Call, C, checkFunc, useFunc);


                    //clear the state for next analysis
                    State = State->set<SCFS>(param, VarState(VarState::getUnChecked()) );
                    C.addTransition(State);
                }
            }
            //if symbol
            else{
                SymbolRef param = sv.getAsSymbol();


                ProgramStateRef State = C.getState();
                const VarState *S = State->get<CFS>(param);
                if(S && S->isChecked()){
                    
                    string checkFunc = S->getCheckFunc();
                    string useFunc = Call.getCalleeIdentifier()->getName();

                    reportTOCTOU(Call, C, checkFunc, useFunc);


                    //clear the state for next analysis
                    State = State->set<CFS>(param, VarState(VarState::getUnChecked()) );
                    C.addTransition(State);
                }
            } 

    }

}
void TOCTOUChecker::reportTOCTOU(const CallEvent &Call, CheckerContext &C, string checkFunc, string useFunc) const{
    ExplodedNode *ErrNode = C.generateSink();
    if(!ErrNode)
        return;

    string ErrMsg = "Calling " + useFunc + "() after " + checkFunc + "() can cause a TOCTOU race condition";
    BugReport *R = new BugReport(*TOCTOUBugType, ErrMsg, ErrNode);
    R->addRange(Call.getSourceRange());
    C.emitReport(R);

}

void ento::registerTOCTOUChecker(CheckerManager &mgr){
    mgr.registerChecker<TOCTOUChecker>();
}



