#pragma once

// universal FSM core
#include <Poseidon/Foundation/Containers/StaticArray.hpp>
#include <Poseidon/Foundation/Time/Time.hpp>
#include <Poseidon/IO/Serialization/ParamArchive.hpp>



namespace Poseidon
{
class FSM: public SerializeClass
{
	public:
	typedef int State;
	enum {FinalState=-1};

	typedef void Context;

	typedef void StateFunction( Context *context );
	typedef StateFunction * pStateFunction;

	class StateInfo
	{
		protected:
		friend class FSM;
		friend class StateArray;
		const char *_name;
		StateFunction *_init;
		StateFunction *_check;
		
		StateInfo(){}
		StateInfo( const char *name, StateFunction *init, StateFunction *check )
		:_name(name),_init(init),_check(check)
		{}
		// State handlers take typed context pointers (e.g. AIGroupContext*) but the FSM
		// stores them as void(*)(void*). The ABI is identical on all targets; suppress
		// UBSan's function-type check rather than forcing wrapper boilerplate on every
		// state in the codebase.
		__attribute__((no_sanitize("function")))
		void Init( Context *context ) const {_init(context);}
		__attribute__((no_sanitize("function")))
		void Check( Context *context ) const {_check(context);}
		const char *GetName() const {return _name;}
	};

	enum {MaxVar=8};
	
	private:
	State _curState;
	const StateInfo *_state;
	const pStateFunction *_special;	// special functions
																		// 0 - entry
																		// 1 - exit
	Foundation::Time _timeOut;
	int _iVar[MaxVar];
	Foundation::Time _tVar[MaxVar];

	int _nStates;
	int _nSpecials;

	const StateInfo &CurState() const
	{
		PoseidonAssert( _curState>=0 && _curState<_nStates );
		return _state[_curState];
	}

	public:
	void SetState( State state, Context *context );
	void SetState( State state ) {_curState = state;}
	State GetState() const {return _curState;}
	const char *GetStateName() const {return CurState().GetName();}

	// variables
	int &Var( int i ) {return _iVar[i];}
	int Var( int i ) const {return _iVar[i];}

	Foundation::Time &VarTime( int i ) {return _tVar[i];}
	Foundation::Time VarTime( int i ) const {return _tVar[i];}

	void SetTimeOut( float sec );
	float GetTimeOut() const;
	bool TimedOut() const;

	FSM();	
	FSM
	(
		const StateInfo *states, int n,
		const pStateFunction *functions = nullptr, int nFunc = 0
	); // typical contructor - static state space
	virtual ~FSM();

	void Init
	(
		const StateInfo *states, int n,
		const pStateFunction *functions, int nFunc
	);
	bool Update( Context *context ); // true -> final state reached
	void Refresh( Context *context ); // use only when FSM state is supposed to be lost
	void Enter( Context *context );
	void Exit( Context *context );

	LSError Serialize(ParamArchive &ar) override;
};

// type-safe implementation of FSM
template <class ContextType>
class FSMTyped: public FSM
{
	public:
	typedef void StateFunction( ContextType *context );
	typedef StateFunction * pStateFunction;
	class StateInfo: public FSM::StateInfo
	{
		public:
		StateInfo( const char *name, StateFunction *init, StateFunction *check )
		:FSM::StateInfo(name,(FSM::StateFunction *)init,(FSM::StateFunction *)check)
		{}
	};

	FSMTyped():FSM(){}
	FSMTyped
	(
		const StateInfo *states, int n,
		const pStateFunction *functions = nullptr, int nFunc = 0
	);
	~FSMTyped() override{}

	void SetState( State state, ContextType *context ) {FSM::SetState(state,context);}
	bool Update( ContextType *context ) {return FSM::Update(context);}
};

template <class ContextType>
FSMTyped<ContextType>::FSMTyped
(
	const StateInfo *states, int n,
	const pStateFunction *functions, int nFunc
)
:FSM(states,n,(const FSM::pStateFunction *)functions,nFunc)
{
}
} // namespace Poseidon
