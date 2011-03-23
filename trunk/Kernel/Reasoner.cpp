/* This file is part of the FaCT++ DL reasoner
Copyright (C) 2003-2011 by Dmitry Tsarkov

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Reasoner.h"
#include "logging.h"
#include "eFPPTimeout.h"

// comment the line out for printing tree info before save and after restoring
//#define __DEBUG_SAVE_RESTORE
// comment the line out for flushing LL after dumping significant piece of info
//#define __DEBUG_FLUSH_LL

#ifdef USE_REASONING_STATISTICS
AccumulatedStatistic* AccumulatedStatistic::root = NULL;
#endif

DlSatTester :: DlSatTester ( TBox& tbox, const ifOptionSet* Options )
	: tBox(tbox)
	, DLHeap(tbox.DLHeap)
	, Manager(64)
	, CGraph(1,this)
	, DTReasoner(tbox.DLHeap)
	// It's unsafe to have a cache that touchs a nominal in a node; set flagNominals to prevent it
	, newNodeCache ( true, tBox.nC, tBox.nR )
	, newNodeEdges ( false, tBox.nC, tBox.nR )
	, GCIs(tbox.GCIs)
	, testTimeout(0)
	, bContext(NULL)
	, tryLevel(InitBranchingLevelValue)
	, nonDetShift(0)
	, curNode(NULL)
	, dagSize(0)
	, duringClassification(false)
{
	// init local options
	readConfig ( Options );

	// in presence of fairness constraints use ancestor blocking
	if ( tBox.hasFC() && useAnywhereBlocking )
	{
		useAnywhereBlocking = false;
		if ( LLM.isWritable(llAlways) )
			LL << "Fairness constraints: set useAnywhereBlocking = false\n";
	}

	// init static part of CTree
	CGraph.initContext ( useLazyBlocking, useAnywhereBlocking );
	// init datatype reasoner
	tBox.getDataTypeCenter().initDataTypeReasoner(DTReasoner);
	// init set of reflexive roles
	tbox.getORM()->fillReflexiveRoles(ReflexiveRoles);
	// init blocking statistics
	clearBlockingStat();

	useActiveSignature = tBox.getSplits() != NULL;
	if ( useActiveSignature )
	{
		initSplits();
		// make entity map
		const size_t size = DLHeap.size();
		EntityMap.resize(size);
		EntityMap[0] = EntityMap[1] = NULL;
		for ( size_t i = 2; i < size-1; ++i )
			if ( DLHeap[i].getConcept() != NULL )
				EntityMap[i] = DLHeap[i].getConcept()->getEntity();
			else
				EntityMap[i] = NULL;
		EntityMap[size-1] = NULL;	// query concept
	}

	resetSessionFlags ();
}

// load init values from config file
void DlSatTester :: readConfig ( const ifOptionSet* Options )
{
	fpp_assert ( Options != NULL );	// safety check

// define a macro for registering boolean option
#	define addBoolOption(name)				\
	name = Options->getBool ( #name );		\
	if ( LLM.isWritable(llAlways) )			\
		LL << "Init " #name " = " << name << "\n"
	addBoolOption(useSemanticBranching);
	addBoolOption(useBackjumping);
	addBoolOption(useLazyBlocking);
	addBoolOption(useAnywhereBlocking);
#undef addBoolOption
}

/// prerpare Nominal Reasoner to a new job
void
DlSatTester :: prepareReasoner ( void )
{
	CGraph.clear();
	Stack.clear();
	TODO.clear();

	pUsed.clear();
	nUsed.clear();
	SessionGCIs.clear();
	ActiveSplits.clear();
	ActiveSignature.clear();

	curNode = NULL;
	bContext = NULL;
	tryLevel = InitBranchingLevelValue;

	// clear last session information
	resetSessionFlags();
}

DlSatTester::SigSet
DlSatTester :: buildSet ( const TSignature& sig, const TNamedEntity* entity )
{
	SigSet Set;
//	std::cout << "Building set for " << entity->getName() << "\n";
	for ( TSignature::iterator p = sig.begin(), p_end = sig.end(); p != p_end; ++p )
		if ( *p != entity && dynamic_cast<const TDLConceptName*>(*p) != NULL )
		{
//			std::cout << "In the set: " << (*p)->getName() << "\n";
			Set.insert(*p);
		}
//	std::cout << "done\n";
	return Set;
}

void
DlSatTester :: initSplit ( const TSplitVar* split )
{
//	std::cout << "Processing split for " << split->oldName->getName() << ":\n";
	SigSet impSet = buildSet ( split->Sigs[0], split->splitNames[0] );
	BipolarPointer bp = split->C->pBody+1;	// choose-rule stays next to a split-definition of C
	for ( size_t i = 1; i < split->Sigs.size(); ++i )
		addSplitRule ( buildSet ( split->Sigs[i], split->splitNames[i] ), impSet, bp );
}

bool
DlSatTester :: findConcept ( const CWDArray& lab, BipolarPointer p )
{
#ifdef ENABLE_CHECKING
	fpp_assert ( isCorrect(p) );	// sanity checking
	// constants are not allowed here
	fpp_assert ( p != bpTOP );
	fpp_assert ( p != bpBOTTOM );
#endif

	incStat(nLookups);
	return lab.contains(p);
}

bool
DlSatTester :: findConceptClash ( const CWDArray& lab, BipolarPointer p, const DepSet& dep )
{
#ifdef ENABLE_CHECKING
	fpp_assert ( isCorrect(p) );	// sanity checking
	// constants are not allowed here
	fpp_assert ( p != bpTOP );
	fpp_assert ( p != bpBOTTOM );
#endif

	incStat(nLookups);

	for ( const_label_iterator i = lab.begin(), i_end = lab.end(); i < i_end; ++i )
		if ( i->bp() == p )
		{
			// create clashSet
			clashSet = i->getDep();
			clashSet.add(dep);
			return true;
		}

	// we are able to insert a concept
	return false;
}

addConceptResult
DlSatTester :: checkAddedConcept ( const CWDArray& lab, BipolarPointer p, const DepSet& dep )
{
#ifdef ENABLE_CHECKING
	fpp_assert ( isCorrect(p) );	// sanity checking
	// constants are not allowed here
	fpp_assert ( p != bpTOP );
	fpp_assert ( p != bpBOTTOM );
#endif

	if ( findConcept ( lab, p ) )
		return acrExist;

	if ( findConceptClash ( lab, inverse(p), dep ) )
		return acrClash;

	// we are able to insert a concept
	return acrDone;
}

addConceptResult
DlSatTester :: tryAddConcept ( const CWDArray& lab, BipolarPointer bp, const DepSet& dep )
{
	// check whether C or ~C can occurs in a node label
	register const BipolarPointer inv_p = inverse(bp);
	bool canC = isUsed(bp);
	bool canNegC = isUsed(inv_p);

	// if either C or ~C is used already, it's not new in a label
	if ( canC )
	{
		if ( canNegC )	// both C and ~C can be in the label
			return checkAddedConcept ( lab, bp, dep );
		else			// C but not ~C can be in the label
			return findConcept ( lab, bp ) ? acrExist : acrDone;
	}
	else
	{
		if ( canNegC )	// ~C but not C can be in the label
			return findConceptClash ( lab, inv_p, dep ) ? acrClash : acrDone;
		else			// neither C nor ~C can be in the label
			return acrDone;
	}
}

bool
DlSatTester :: addToDoEntry ( DlCompletionTree* node, const ConceptWDep& C, const char* reason )
{
	if ( C == bpTOP )	// simplest things first
		return false;
	if ( C == bpBOTTOM )
	{
		setClashSet(C.getDep());
		if ( LLM.isWritable(llGTA) )
			logClash ( node, C );
		return true;
	}

	const DLVertex& v = DLHeap[C];
	DagTag tag = v.Type();

	// collections shouldn't appear in the node labels
	if ( tag == dtCollection )
	{
		if ( isNegative(C.bp()) )	// nothing to do
			return false;
		// setup and run and()
		incStat(nTacticCalls);		// to balance nAndCalls later
		DlCompletionTree* oldNode = curNode;
		ConceptWDep oldConcept = curConcept;
		curNode = node;
		curConcept = C;
		bool ret = commonTacticBodyAnd(v);
		curNode = oldNode;
		curConcept = oldConcept;
		return ret;
	}

	// try to add a concept to a node label
	switch ( tryAddConcept ( node->label().getLabel(tag), C.bp(), C.getDep() ) )
	{
	case acrClash:	// clash -- return
		if ( LLM.isWritable(llGTA) )
			logClash ( node, C );
		return true;
	case acrExist:	// already exists -- nothing new
		return false;
	case acrDone:	// try was done
		return insertToDoEntry ( node, C, tag, reason );
	default:	// safety check
		fpp_unreachable();
	}
}

/// insert P to the label of N; do necessary updates; may return Clash in case of data node N
bool
DlSatTester :: insertToDoEntry ( DlCompletionTree* node, const ConceptWDep& C,
								 DagTag tag, const char* reason = NULL )
{
	// we will change current Node => save it if necessary
	updateLevel ( node, C.getDep() );
	CGraph.addConceptToNode ( node, C, tag );

	setUsed(C.bp());

	if ( useActiveSignature )
		if ( updateActiveSignature ( getEntity(C.bp()), C.getDep() ) )
			return true;

	if ( node->isCached() )
		return correctCachedEntry(node);

	// add new info in TODO list
	TODO.addEntry ( node, tag, C );

	if ( node->isDataNode() )	// data concept -- run data center for it
		return checkDataNode ? checkDataClash(node) : false;
	else if ( LLM.isWritable(llGTA) )	// inform about it
		logEntry ( node, C, reason );

	return false;
}

//-----------------------------------------------------------------------------
//--		internal cache support
//-----------------------------------------------------------------------------

bool
DlSatTester :: canBeCached ( DlCompletionTree* node )
{
	DlCompletionTree::const_label_iterator p;
	bool shallow = true;
	unsigned int size = 0;

	// nominal nodes can not be cached
	if ( node->isNominalNode() )
		return false;

	incStat(nCacheTry);

	// check applicability of the caching
	for ( p = node->beginl_sc(); p != node->endl_sc(); ++p )
	{
		if ( DLHeap.getCache(p->bp()) == NULL )
		{
			incStat(nCacheFailedNoCache);
			if ( LLM.isWritable(llGTA) )
				LL << " cf(" << p->bp() << ")";
			return false;
		}

		shallow &= DLHeap.getCache(p->bp())->shallowCache();
		++size;
	}

	for ( p = node->beginl_cc(); p != node->endl_cc(); ++p )
	{
		if ( DLHeap.getCache(p->bp()) == NULL )
		{
			incStat(nCacheFailedNoCache);
			if ( LLM.isWritable(llGTA) )
				LL << " cf(" << p->bp() << ")";
			return false;
		}

		shallow &= DLHeap.getCache(p->bp())->shallowCache();
		++size;
	}

	// it's useless to cache shallow nodes
	if ( shallow && size != 0 )
	{
		incStat(nCacheFailedShallow);
		if ( LLM.isWritable(llGTA) )
			LL << " cf(s)";
		return false;
	}

	return true;
}

/// perform caching of the node (it is known that caching is possible)
void
DlSatTester :: doCacheNode ( DlCompletionTree* node )
{
	DlCompletionTree::const_label_iterator p;
	DepSet dep;

	newNodeCache.clear();

	for ( p = node->beginl_sc(); p != node->endl_sc(); ++p )
	{
		dep.add(p->getDep());
		// try to merge cache of a node label element with accumulator
		switch ( newNodeCache.merge(DLHeap.getCache(p->bp())) )
		{
		case csValid:	// continue
			break;
		case csInvalid:	// clash: set the clash-set
			setClashSet(dep);
		// fall through
		default:		// caching of node fails
			return;
		}
	}

	for ( p = node->beginl_cc(); p != node->endl_cc(); ++p )
	{
		dep.add(p->getDep());
		// try to merge cache of a node label element with accumulator
		switch ( newNodeCache.merge(DLHeap.getCache(p->bp())) )
		{
		case csValid:	// continue
			break;
		case csInvalid:	// clash: set the clash-set
			setClashSet(dep);
		// fall through
		default:		// caching of node fails
			return;
		}
	}

	// all concepts in label are mergable; now try to add input arc
	newNodeEdges.clear();
	newNodeEdges.initRolesFromArcs(node);
	newNodeCache.merge(&newNodeEdges);
}

modelCacheState
DlSatTester :: reportNodeCached ( DlCompletionTree* node )
{
	doCacheNode(node);
	enum modelCacheState status = newNodeCache.getState();
	switch ( status )
	{
	case csValid:
		incStat(nCachedSat);
		if ( LLM.isWritable(llGTA) )
			LL << " cached(" << node->getId() << ")";
		break;
	case csInvalid:
		incStat(nCachedUnsat);
		break;
	case csFailed:
	case csUnknown:
		incStat(nCacheFailed);
		if ( LLM.isWritable(llGTA) )
			LL << " cf(c)";
		status = csFailed;
		break;
	default:
		fpp_unreachable();
	}
	return status;
}

bool DlSatTester :: correctCachedEntry ( DlCompletionTree* n )
{
	fpp_assert ( n->isCached() );	// safety check

	// FIXME!! check if it is possible to leave node cached in more efficient way
	modelCacheState status = tryCacheNode(n);

	// uncheck cached node status and add all elements in TODO list
	if ( status == csFailed )
		redoNodeLabel ( n, "uc" );

	return usageByState(status);
}

//-----------------------------------------------------------------------------
//--		internal datatype support
//-----------------------------------------------------------------------------

/// @return true iff given data node contains data contradiction
bool
DlSatTester :: hasDataClash ( const DlCompletionTree* Node )
{
	fpp_assert ( Node && Node->isDataNode() );	// safety check

	DTReasoner.clear();

	// data node may contain only "simple" concepts in there
	for ( DlCompletionTree::const_label_iterator r = Node->beginl_sc(); r != Node->endl_sc(); ++r )
		if ( DTReasoner.addDataEntry ( r->bp(), r->getDep() ) )	// clash found
			return true;

	return DTReasoner.checkClash();
}

bool DlSatTester :: runSat ( void )
{
	testTimer.Start();
	bool result = checkSatisfiability ();
	testTimer.Stop();

	if ( LLM.isWritable(llSatTime) )
		LL << "\nChecking time was " << testTimer << " seconds";

	testTimer.Reset();

	finaliseStatistic();

	if ( result )
		writeRoot(llRStat);

	return result;
}

void
DlSatTester :: finaliseStatistic ( void )
{
#ifdef USE_REASONING_STATISTICS
	// add the integer stat values
	nNodeSaves.set(CGraph.getNNodeSaves());
	nNodeRestores.set(CGraph.getNNodeRestores());

	// log statistics data
	if ( LLM.isWritable(llRStat) )
		logStatisticData ( LL, /*needLocal=*/true );

	// merge local statistics with the global one
	AccumulatedStatistic::accumulateAll();
#endif

	// clear global statistics
	CGraph.clearStatistics();
}

bool DlSatTester :: applyReflexiveRoles ( DlCompletionTree* node, const DepSet& dep )
{
	for ( TRole::const_iterator p = ReflexiveRoles.begin(), p_end = ReflexiveRoles.end(); p != p_end; ++p )
	{
		// create R-loop through the NODE
		DlCompletionTreeArc* pA = CGraph.addRoleLabel ( node, node, /*isPredEdge=*/false, *p, dep );
		if ( setupEdge ( pA, dep, 0 ) )
			return true;
	}

	// no clash found
	return false;
}

bool DlSatTester :: checkSatisfiability ( void )
{
	unsigned int loop = 0;
	for (;;)
	{
		if ( curNode == NULL )
		{
			if ( TODO.empty() )	// nothing more to do
			{	// make sure all blocked nodes are still blocked
				if ( LLM.isWritable(llGTA) )
				{
					logIndentation();
					LL << "[*ub:";
				}
				CGraph.retestCGBlockedStatus();
				if ( LLM.isWritable(llGTA) )
					LL << "]";
				if ( TODO.empty() )
#				ifndef RKG_USE_FAIRNESS
					return true;
#				else
				{	// check fairness constraints
					if ( !tBox.hasFC() )
						return true;
					// reactive fairness: for every given FC, if it is violated, reject current model
					for ( TBox::ConceptVector::const_iterator p = tBox.Fairness.begin(), p_end = tBox.Fairness.end(); p < p_end; ++p )
						if ( CGraph.isFCViolated((*p)->pName) )
						{
							nFairnessViolations.inc();
							if ( straightforwardRestore() )	// no more branching alternatives
								return false;
							else
								break;
						}

					if ( TODO.empty() )
						return true;
				}
#				endif
			}

			const ToDoEntry* curTDE = TODO.getNextEntry ();
			fpp_assert ( curTDE != NULL );

			// setup current context
			curNode = curTDE->Node;
			curConcept = curNode->label().getConcept(curTDE->offset);
		}

		if ( ++loop == 5000 )
		{
			loop = 0;
			if ( tBox.isCancelled() )
				return false;
			if ( testTimeout && 1000*(float)testTimer >= testTimeout )
				throw EFPPTimeout();
		}
		// here curNode/curConcept are set
		if ( commonTactic() )	// clash found
		{
			if ( tunedRestore() )	// the concept is unsatisfiable
				return false;
		}
		else
			curNode = NULL;
	}
}

/********************************************************************************
  *
  *  Save/Restore section
  *
  ******************************************************************************/

	/// restore local state from BContext
void DlSatTester :: restoreBC ( void )
{
	// restore reasoning context
	curNode = bContext->curNode;
	curConcept = bContext->curConcept;
	pUsed.resize(bContext->pUsedIndex);
	nUsed.resize(bContext->nUsedIndex);
	if ( unlikely ( !SessionGCIs.empty() ) )
		SessionGCIs.resize(bContext->SGsize);

	// update branch dep-set
	updateBranchDep();
	bContext->nextOption();
}

void DlSatTester :: save ( void )
{
	// save tree
	CGraph.save();

	// save ToDoList
	TODO.save();

	// increase tryLevel
	++tryLevel;
	Manager.ensureLevel(getCurLevel());

	// init BC
	clearBC();

	incStat(nStateSaves);

	if ( LLM.isWritable(llSRState) )
		LL << " ss(" << getCurLevel()-1 << ")";
#ifdef __DEBUG_SAVE_RESTORE
	writeRoot(llSRState);
#endif
}

void DlSatTester :: restore ( unsigned int newTryLevel )
{
	fpp_assert ( !Stack.empty () );
	fpp_assert ( newTryLevel > 0 );

	// skip all intermediate restorings
	setCurLevel(newTryLevel);

	// restore local
	bContext = Stack.top(getCurLevel());
	restoreBC();

	// restore tree
	CGraph.restore(getCurLevel());

	// restore TODO list
	TODO.restore(getCurLevel());

	incStat(nStateRestores);

	if ( LLM.isWritable(llSRState) )
		LL << " sr(" << getCurLevel() << ")";
#ifdef __DEBUG_SAVE_RESTORE
	writeRoot(llSRState);
#endif
}

/**
  * logging methods
  */

void DlSatTester :: logIndentation ( void ) const
{
	CHECK_LL_RETURN(llGTA);	// useless but safe
	LL << "\n";
	for ( register unsigned int i = getCurLevel(); --i; )
		LL << " ";
}
void DlSatTester :: logStartEntry ( void ) const
{
	CHECK_LL_RETURN(llGTA);	// useless but safe
	logIndentation();
	LL << "[*(";
	curNode->logNode ();
	LL << "," << curConcept << "){";
	if ( isNegative (curConcept.bp()) )
		LL << "~";
	LL << DLHeap[curConcept].getTagName() << "}:";
}

void DlSatTester :: logFinishEntry ( bool res ) const
{
	CHECK_LL_RETURN(llGTA);	// useless but safe

	LL << "]";
	if ( res )
		LL << " Clash" << getClashSet();
#ifdef __DEBUG_FLUSH_LL
	LL.flush ();
#endif
}

void DlSatTester :: logStatisticData ( std::ostream& o, bool needLocal ) const
{
#ifdef USE_REASONING_STATISTICS
	nTacticCalls.Print	( o, needLocal, "\nThere were made ", " tactic operations, of which:" );
	nIdCalls.Print		( o, needLocal, "\n    CN   operations: ", "" );
	nSingletonCalls.Print(o, needLocal, "\n           including ", " singleton ones" );
	nOrCalls.Print		( o, needLocal, "\n    OR   operations: ", "" );
	nOrBrCalls.Print	( o, needLocal, "\n           ", " of which are branching" );
	nAndCalls.Print		( o, needLocal, "\n    AND  operations: ", "" );
	nSomeCalls.Print	( o, needLocal, "\n    SOME operations: ", "" );
	nAllCalls.Print		( o, needLocal, "\n    ALL  operations: ", "" );
	nFuncCalls.Print	( o, needLocal, "\n    Func operations: ", "" );
	nLeCalls.Print		( o, needLocal, "\n    LE   operations: ", "" );
	nGeCalls.Print		( o, needLocal, "\n    GE   operations: ", "" );
	nUseless.Print		( o, needLocal, "\n    N/A  operations: ", "" );

	nNNCalls.Print		( o, needLocal, "\nThere were made ", " NN rule application" );
	nMergeCalls.Print	( o, needLocal, "\nThere were made ", " merging operations" );

	nAutoEmptyLookups.Print	( o, needLocal, "\nThere were made ", " RA empty transition lookups" );
	nAutoTransLookups.Print	( o, needLocal, "\nThere were made ", " RA applicable transition lookups" );

	nSRuleAdd.Print		( o, needLocal, "\nThere were made ", " simple rule additions" );
	nSRuleFire.Print	( o, needLocal, "\n       of which ", " simple rules fired" );

	nStateSaves.Print		( o, needLocal, "\nThere were made ", " save(s) of global state" );
	nStateRestores.Print	( o, needLocal, "\nThere were made ", " restore(s) of global state" );
	nNodeSaves.Print		( o, needLocal, "\nThere were made ", " save(s) of tree state" );
	nNodeRestores.Print		( o, needLocal, "\nThere were made ", " restore(s) of tree state" );
	nLookups.Print			( o, needLocal, "\nThere were made ", " concept lookups" );
#ifdef RKG_USE_FAIRNESS
	nFairnessViolations.Print	( o, needLocal, "\nThere were ", " fairness constraints violation" );
#endif

	nCacheTry.Print				( o, needLocal, "\nThere were made ", " tries to cache completion tree node, of which:" );
	nCacheFailedNoCache.Print	( o, needLocal, "\n                ", " fails due to cache absence" );
	nCacheFailedShallow.Print	( o, needLocal, "\n                ", " fails due to shallow node" );
	nCacheFailed.Print			( o, needLocal, "\n                ", " fails due to cache merge failure" );
	nCachedSat.Print			( o, needLocal, "\n                ", " cached satisfiable nodes" );
	nCachedUnsat.Print			( o, needLocal, "\n                ", " cached unsatisfiable nodes" );
#endif

	if ( !needLocal )
		o << "\nThe maximal graph size is " << CGraph.maxSize() << " nodes";
}

float
DlSatTester :: printReasoningTime ( std::ostream& o ) const
{
	o << "\n     SAT takes " << satTimer << " seconds\n     SUB takes " << subTimer << " seconds";
	return satTimer + subTimer;
}

