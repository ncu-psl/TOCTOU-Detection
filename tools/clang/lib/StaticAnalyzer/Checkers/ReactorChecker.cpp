#include "ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"

using namespace clang;
using namespace ento;

class ReactorState{
private:
    enum Kind {On, Off} K;

public:
    ReactorState(unsigned InK): K( (Kind) InK){}
    bool isOn()const {return K == On;}
    bool isOff()const {return K == Off;}
    static unsigned getOn() {return (unsigned) On;}
    static unsigned getOff() {return (unsigned) Off;}
    bool operator==(const ReactorState &X) const{
        return K == X.K;    
    }
    void Profile (llvm::FoldingSetNodeID &ID) const{
        ID.AddInteger(K);
    }

}; 



class ReactorChecker : public Checker<check::PostCall> {
    mutable IdentifierInfo *IIturnReactionOn, *IISCRAM;
    OwningPtr<BugType> DoubleONBugType;
    OwningPtr<BugType> DoubleSCRAMBugType;

    void initIdentifierInfo(ASTContext &Ctx) const;
    void reportDoubleSCRAM(const CallEvent &Call, CheckerContext &C) const;
    void reportDoubleON(const CallEvent &Call, CheckerContext &C) const;

public:
    ReactorChecker();
    void checkPostCall(const CallEvent &Call, CheckerContext &C) const;
};

REGISTER_MAP_WITH_PROGRAMSTATE(RS, int, ReactorState)




ReactorChecker::ReactorChecker() : IIturnReactionOn(0), IISCRAM(0) {
    DoubleONBugType.reset( new BugType("Double ON", "Nuclear Reactor API Error") );
    DoubleSCRAMBugType.reset( new BugType("Double SCRAM", "Nuclear Reactor API Error") );

} 

void ReactorChecker::initIdentifierInfo(ASTContext &Ctx) const{
    if (IIturnReactionOn)
        return;

    IIturnReactionOn = &Ctx.Idents.get("turnReactorOn");
    IISCRAM = &Ctx.Idents.get("SCRAM");
}

void ReactorChecker::checkPostCall(const CallEvent &Call, CheckerContext &C) const{
    
    initIdentifierInfo(C.getASTContext());

    if(!Call.isGlobalCFunction())
        return;

    if(Call.getCalleeIdentifier() == IIturnReactionOn){
        ProgramStateRef State = C.getState();
        const ReactorState *S = State->get<RS>(1);

        if(S && S->isOn()){
            reportDoubleON(Call, C);
            return;
        }
        State = State->set<RS>(1, ReactorState::getOn());
        C.addTransition(State);
        return;
    }

    if(Call.getCalleeIdentifier() == IISCRAM){
        ProgramStateRef State = C.getState();
        const ReactorState *S = State->get<RS>(1);

        if(S && S->isOff()){
            reportDoubleSCRAM(Call, C);
            return;
        }
        State = State->set<RS>(1, ReactorState::getOff());
        C.addTransition(State);
        return;
    }
}

void ReactorChecker::reportDoubleON(const CallEvent &Call, CheckerContext &C) const{

    ExplodedNode *ErrNode = C.generateSink();
    if(!ErrNode)
        return;

    BugReport *R = new BugReport(*DoubleONBugType, "Turn on two times", ErrNode);
    R->addRange(Call.getSourceRange());
    R->markInteresting(Call.getReturnValue().getAsSymbol());
    C.emitReport(R);
}

void ReactorChecker::reportDoubleSCRAM(const CallEvent &Call, CheckerContext &C) const{

    ExplodedNode *ErrNode = C.generateSink();
    if(!ErrNode)
        return;

    BugReport *R = new BugReport(*DoubleSCRAMBugType, "SCRAM two times", ErrNode);
    R->addRange(Call.getSourceRange());
    R->markInteresting(Call.getReturnValue().getAsSymbol());
    C.emitReport(R);
}

void ento::registerReactorChecker(CheckerManager &mgr){
    mgr.registerChecker<ReactorChecker>();
}



