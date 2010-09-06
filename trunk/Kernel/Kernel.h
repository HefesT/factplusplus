/* This file is part of the FaCT++ DL reasoner
Copyright (C) 2003-2010 by Dmitry Tsarkov

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef KERNEL_H
#define KERNEL_H

#include <string>

#include "fpp_assert.h"
#include "eFPPInconsistentKB.h"
#include "tNAryQueue.h"
#include "dlTBox.h"
#include "ReasonerNom.h"
#include "ifOptions.h"
#include "DLConceptTaxonomy.h"	// for getRelatives()
#include "tExpressionTranslator.h"
#include "tOntology.h"

class ReasoningKernel
{
public:	// types interface
	/*
		The type system for DL expressions used in the input language:

		TExpr;
		 TConceptExpr: TExpr;
		 TIndividualExpr : TExpr;
		 TRoleExpr: TExpr;
		  TORoleComplexExpr: TRoleExpr;
		   TORoleExpr: TORoleComplexExpr;
		  TDRoleExpr: TRoleExpr;
		 TDataExpr: TExpr;
		  TDataValueExpr: TDataExpr;

		Right now all the expressions are of the type DLTree*.
		Later on the appropriate classes from tDlExpression.h

		As a transition step, the intermediate structure is introduced.
	*/

		/// general expression
	typedef const TDLExpression TExpr;
		/// concept expression
	typedef const TDLConceptExpression TConceptExpr;
		/// individual expression
	typedef const TDLIndividualExpression TIndividualExpr;
		/// role expression
	typedef const TDLRoleExpression TRoleExpr;
		/// object role complex expression (including role chains and projections)
	typedef const TDLObjectRoleComplexExpression TORoleComplexExpr;
		/// object role expression
	typedef const TDLObjectRoleExpression TORoleExpr;
		/// data role expression
	typedef const TDLDataRoleExpression TDRoleExpr;
		/// data expression
	typedef const TDLDataExpression TDataExpr;
		/// data type expression
	typedef TDLDataTypeExpression TDataTypeExpr;
		/// data value expression
	typedef const TDLDataValue TDataValueExpr;
		/// data facet expression
	typedef const TDLFacetExpression TFacetExpr;

	// name sets

		/// set of arbitrary named expressions
	typedef std::vector<const TNamedEntry*> NamesVector;
		// IndividualSet is just set of named individual expressions
	typedef NamesVector IndividualSet;

		/// typedef for intermediate instance related type
	typedef TRelatedMap::CIVec CIVec;

private:
		/// options for the kernel and all related substructures
	ifOptionSet KernelOptions;

private:
	static const char* Version;
	static const char* ProductName;
	static const char* Copyright;
	static const char* ReleaseDate;
		/// header of the file with internal state; defined in SaveLoad.cpp
	static const char* InternalStateFileHeader;

protected:	// types
		/// enumeration for the cache
	enum cacheStatus { csEmpty, csSat, csClassified };
		/// set of TreeNE
/*	class TreeNESet: public TNameSet<TTreeNamedEntry>
	{
	public:
			/// dirty hack for the LISP ontology printing
		template<class T>
		void fill ( T& x ) const
		{
			for ( const_iterator p = Base.begin(); p != Base.end(); ++p )
				x.recordDataRole(p->second->getName());
		}
	}; // TreeNESet
*/
protected:	// members
		/// local TBox (to be created)
	TBox* pTBox;
		/// set of axioms
	TOntology Ontology;
		/// expression translator to work with queries
	TExpressionTranslator* pET;

	// Top/Bottom role names: if set, they will appear in all hierarchy-related output

		/// top object role name
	std::string TopORoleName;
		/// bottom object role name
	std::string BotORoleName;
		/// top data role name
	std::string TopDRoleName;
		/// bottom data role name
	std::string BotDRoleName;

	// values to propagate to the new KB in case of clearance

		/// progress monitor (if any)
	TProgressMonitor* pMonitor;
		/// timeout value
	unsigned long OpTimeout;
		/// tell reasoner to use verbose output
	bool verboseOutput;

	// reasoning cache

		/// cache level
	enum cacheStatus cacheLevel;
		/// cached query concept description
	DLTree* cachedQuery;
		/// cached concept (either defConcept or existing one)
	TConcept* cachedConcept;
		/// cached query result (taxonomy position)
	TaxonomyVertex* cachedVertex;

	// internal flags

		/// set if TBox throws an exception during preprocessing/classification
	bool reasoningFailed;

private:	// no copy
		/// no copy c'tor
	ReasoningKernel ( const ReasoningKernel& );
		/// no assignment
	ReasoningKernel& operator = ( const ReasoningKernel& );

protected:	// methods

	// register all necessary options in local option set
	bool initOptions ( void );

		/// get status of the KB
	KBStatus getStatus ( void ) const
	{
		if ( pTBox == NULL )
			return kbEmpty;
		// if the ontology is changed, it needs to be reclassified
		if ( Ontology.isChanged() )
			return kbLoading;
		return pTBox->getStatus();
	}
		/// process KB wrt STATUS
	void processKB ( KBStatus status );

		/// get DLTree corresponding to an expression EXPR
	DLTree* e ( const TExpr* expr )
	{
		fpp_assert ( pET != NULL );
		expr->accept(*pET);
		return *pET;
	}
		/// set up cache for query, performing additional (re-)classification if necessary
	void setUpCache ( DLTree* query, cacheStatus level );
		/// clear cache and flags
	void initCacheAndFlags ( void )
	{
		cacheLevel = csEmpty;
		deleteTree(cachedQuery);
		cachedQuery = NULL;
		cachedConcept = NULL;
		cachedVertex = NULL;
		reasoningFailed = false;
	}

		/// build and set a cache for an individual I wrt role R
	CIVec buildRelatedCache ( TIndividual* I, const TRole* R );
		/// get related cache for an individual I
	const CIVec& getRelated ( TIndividual* I, const TRole* R )
	{
		if ( !I->hasRelatedCache(R) )
			I->setRelatedCache ( R, buildRelatedCache ( I, R ) );
		return I->getRelatedCache(R);
	}

		/// @return true iff C is satisfiable
	bool checkSat ( DLTree* C )
	{
		if ( isCN(C) )
			return getTBox()->isSatisfiable(getTBox()->getCI(TreeDeleter(C)));

		setUpCache ( C, csSat );
		return getTBox()->isSatisfiable(cachedConcept);
	}
		/// @return true iff C [= D holds
	bool checkSub ( DLTree* C, DLTree* D )
	{
		if ( isCN(C) && isCN(D) )
			return getTBox()->isSubHolds ( getTBox()->getCI(TreeDeleter(C)), getTBox()->getCI(TreeDeleter(D)) );

		return !checkSat ( createSNFAnd ( C, createSNFNot(D) ) );
	}

	// helper methods to query properties of roles

		/// @return true if R is functional wrt ontology
	bool checkFunctionality ( DLTree* R )
	{
		// R is transitive iff \ER.C and \ER.\not C is unsatisfiable
		DLTree* tmp = createSNFExists ( clone(R), createSNFNot(getTBox()->getFreshConcept()) );
		tmp = createSNFAnd ( tmp, createSNFExists ( R, getTBox()->getFreshConcept() ) );
		return !checkSat(tmp);
	}
		/// @return true if R is functional; set the value for R if necessary
	bool getFunctionality ( TRole* R )
	{
		if ( !R->isFunctionalityKnown() )	// calculate functionality
			R->setFunctional ( checkFunctionality ( new DLTree ( TLexeme ( R->isDataRole() ? DNAME : RNAME, R ) ) ) );
		return R->isFunctional();
	}
		/// @return true if R is transitive wrt ontology
	bool checkTransitivity ( DLTree* R )
	{
		// R is transitive iff \ER.\ER.C and \AR.\not C is unsatisfiable
		DLTree* tmp = createSNFExists ( clone(R), createSNFNot(getTBox()->getFreshConcept()) );
		tmp = createSNFExists ( clone(R), tmp );
		tmp = createSNFAnd ( tmp, createSNFForall ( R, getTBox()->getFreshConcept() ) );
		return !checkSat(tmp);
	}
		/// @return true if R is symmetric wrt ontology
	bool checkSymmetry ( DLTree* R )
	{
		// R is symmetric iff C and \ER.\AR.(not C) is unsatisfiable
		DLTree* tmp = createSNFForall ( clone(R), createSNFNot(getTBox()->getFreshConcept()) );
		tmp = createSNFAnd ( getTBox()->getFreshConcept(), createSNFExists ( R, tmp ) );
		return !checkSat(tmp);
	}
		/// @return true if R is reflexive wrt ontology
	bool checkReflexivity ( DLTree* R )
	{
		// R is reflexive iff C and \AR.(not C) is unsatisfiable
		DLTree* tmp = createSNFForall ( R, createSNFNot(getTBox()->getFreshConcept()) );
		tmp = createSNFAnd ( getTBox()->getFreshConcept(), tmp );
		return !checkSat(tmp);
	}
		/// @return true if R [= S wrt ontology
	bool checkRoleSubsumption ( DLTree* R, DLTree* S )
	{
		// R [= S iff \ER.C and \AS.(not C) is unsatisfiable
		DLTree* tmp = createSNFForall ( S, createSNFNot(getTBox()->getFreshConcept()) );
		tmp = createSNFAnd ( createSNFExists ( R, getTBox()->getFreshConcept() ), tmp );
		return !checkSat(tmp);
	}
		/// @return true iff the chain contained in the arg-list is a sub-property of R
	bool checkSubChain ( TRole* R )
	{
		// retrieve a role chain
		typedef const std::vector<const TDLExpression*> TExprVec;
		TExprVec Chain = getExpressionManager()->getArgList();

		// R1 o ... o Rn [= R iff \ER1.\ER2....\ERn.(notC) and AR.C is unsatisfiable
		DLTree* tmp = createSNFNot(getTBox()->getFreshConcept());
		for ( TExprVec::const_reverse_iterator p = Chain.rbegin(), p_end = Chain.rend(); p != p_end; ++p )
		{
			TORoleExpr* Ri = dynamic_cast<TORoleExpr*>(*p);
			if ( Ri == NULL )
				throw EFaCTPlusPlus("Role expression expected in the role chain construct");
			tmp = createSNFExists ( e(Ri), tmp );
		}
		tmp = createSNFAnd ( tmp, createSNFForall ( new DLTree(TLexeme(RNAME,R)), getTBox()->getFreshConcept() ) );
		return !checkSat(tmp);
	}

	// get access to internal structures

		/// @throw an exception if no TBox found
	void checkTBox ( void ) const
	{
		if ( pTBox == NULL )
			throw EFaCTPlusPlus("FaCT++ Kernel: KB Not Initialised");
	}
		/// get RW access to TBox
	TBox* getTBox ( void ) { checkTBox(); return pTBox; }
		/// get RO access to TBox
	const TBox* getTBox ( void ) const { checkTBox(); return pTBox; }
		/// clear TBox and related structures; keep ontology in place
	void clearTBox ( void )
	{
		delete pTBox;
		pTBox = NULL;
		delete pET;
		pET = NULL;
		deleteTree(cachedQuery);
		cachedQuery = NULL;
	}

		/// get RW access to Object RoleMaster from TBox
	RoleMaster* getORM ( void ) { return getTBox()->getORM(); }
		/// get RO access to Object RoleMaster from TBox
	const RoleMaster* getORM ( void ) const { return getTBox()->getORM(); }
		/// get RW access to Data RoleMaster from TBox
	RoleMaster* getDRM ( void ) { return getTBox()->getDRM(); }
		/// get RO access to Data RoleMaster from TBox
	const RoleMaster* getDRM ( void ) const { return getTBox()->getDRM(); }

		/// get access to the concept hierarchy
	Taxonomy* getCTaxonomy ( void )
	{
		if ( !isKBClassified() )
			throw EFaCTPlusPlus("No access to concept taxonomy: ontology not classified");
		return getTBox()->getTaxonomy();
	}
		/// get access to the object role hierarchy
	Taxonomy* getORTaxonomy ( void )
	{
		if ( !isKBPreprocessed() )
			throw EFaCTPlusPlus("No access to the object role taxonomy: ontology not preprocessed");
		return getORM()->getTaxonomy();
	}
		/// get access to the data role hierarchy
	Taxonomy* getDRTaxonomy ( void )
	{
		if ( !isKBPreprocessed() )
			throw EFaCTPlusPlus("No access to the data role taxonomy: ontology not preprocessed");
		return getDRM()->getTaxonomy();
	}

	// transformation methods

		/// get individual by the TIndividualExpr
	TIndividual* getIndividual ( const TIndividualExpr* i, const char* reason )
	{
		try
		{
			TreeDeleter I = e(i);
			if ( I == NULL )
				throw EFaCTPlusPlus(reason);
			return static_cast<TIndividual*>(getTBox()->getCI(I));
		}
		catch(...)
		{
			throw EFaCTPlusPlus(reason);
		}
	}
		/// get role by the TRoleExpr
	TRole* getRole ( const TRoleExpr* r, const char* reason )
	{
		try { return resolveRole(TreeDeleter(e(r))); }
		catch(...) { throw EFaCTPlusPlus(reason); }
	}

		/// get taxonomy of the property wrt it's name
	Taxonomy* getTaxonomy ( TRole* R )
	{
		return R->isDataRole() ? getDRTaxonomy() : getORTaxonomy();
	}
		/// get taxonomy vertex of the property wrt it's name
	TaxonomyVertex* getTaxVertex ( TRole* R )
	{
		return R->getTaxVertex();
	}

	//----------------------------------------------
	//-- save/load support; implementation in SaveLoad.cpp
	//----------------------------------------------

		/// save the header of the kernel
	void SaveHeader ( std::ostream& o ) const;
		/// save the set of Kernel's options
	void SaveOptions ( std::ostream& o ) const;
		/// save the status of the KB and the appropriate part of KB
	void SaveKB ( std::ostream& o ) const;
		/// load the header for the kernel
	bool LoadHeader ( std::istream& i );
		/// load the set of Kernel's options
	void LoadOptions ( std::istream& i );
		/// load the status of the KB and the appropriate part of KB
	void LoadKB ( std::istream& i );

public:	// general staff
		/// default c'tor
	ReasoningKernel ( void );
		/// d'tor
	~ReasoningKernel ( void )
	{
		clearTBox();
		delete pMonitor;
	}

	ifOptionSet* getOptions ( void ) { return &KernelOptions; }
	const ifOptionSet* getOptions ( void ) const { return &KernelOptions; }

	static const char* getVersion ( void ) { return Version; }

		/// return classification status of KB
	bool isKBPreprocessed ( void ) const { return getStatus() >= kbCChecked; }
		/// return classification status of KB
	bool isKBClassified ( void ) const { return getStatus() >= kbClassified; }
		/// return realistion status of KB
	bool isKBRealised ( void ) const { return getStatus() >= kbRealised; }

		/// set Progress monitor to control the classification process
	void setProgressMonitor ( TProgressMonitor* pMon )
	{
		delete pMonitor;
		pMonitor = pMon;
		if ( pTBox != NULL )
			pTBox->setProgressMonitor(pMon);
	}
		/// set verbose output (ie, concept and role taxonomies) wrt given VALUE
	void setVerboseOutput ( bool value )
	{
		verboseOutput = value;
		if ( pTBox != NULL )
			pTBox->setVerboseOutput(value);
	}

		/// set top/bottom role names to use them in the related output
	void setTopBottomRoleNames ( const char* topORoleName, const char* botORoleName, const char* topDRoleName, const char* botDRoleName )
	{
		TopORoleName = topORoleName;
		BotORoleName = botORoleName;
		TopDRoleName = topDRoleName;
		BotDRoleName = botDRoleName;
	}

		/// dump query processing TIME, reasoning statistics and a (preprocessed) TBox
	void writeReasoningResult ( std::ostream& o, float time ) const
		{ getTBox()->writeReasoningResult ( o, time ); }

		/// set timeout value to VALUE
	void setOperationTimeout ( unsigned long value )
	{
		OpTimeout = value;
		if ( pTBox != NULL )
			pTBox->setTestTimeout(value);
	}

	//----------------------------------------------
	//-- save/load interface; implementation in SaveLoad.cpp
	//----------------------------------------------

		/// save internal state of the Kernel to a file NAME
	void Save ( std::ostream& o ) const;
		/// load internal state of the Kernel from a file NAME
	void Load ( std::istream& i );
		/// save internal state of the Kernel to a file NAME
	void Save ( const char* name ) const;
		/// load internal state of the Kernel from a file NAME
	void Load ( const char* name );

		/// get access to an expression manager
	TExpressionManager* getExpressionManager ( void ) { return Ontology.getExpressionManager(); }

public:
	//******************************************
	//* KB Management
	//******************************************

		/// create new KB
	bool newKB ( void )
	{
		if ( pTBox != NULL )
			return true;

		pTBox = new TBox ( getOptions(), TopORoleName, BotORoleName, TopDRoleName, BotDRoleName );
		pTBox->setTestTimeout(OpTimeout);
		pTBox->setProgressMonitor(pMonitor);
		pTBox->setVerboseOutput(verboseOutput);
		pET = new TExpressionTranslator(*pTBox);
		initCacheAndFlags();
		return false;
	}
		/// delete existed KB
	bool releaseKB ( void )
	{
		clearTBox();
		Ontology.clear();

		return false;
	}
		/// reset current KB
	bool clearKB ( void )
	{
		if ( pTBox == NULL )
			return true;
		return releaseKB () || newKB ();
	}

	//----------------------------------------------------
	//	TELLS interface
	//----------------------------------------------------

	// Declaration axioms

		/// axiom declare(x)
	TDLAxiom* declare ( TExpr* C ) { return Ontology.add(new TDLAxiomDeclaration(C)); }

	// Concept axioms

		/// axiom C [= D
	TDLAxiom* impliesConcepts ( TConceptExpr* C,TConceptExpr* D )
		{ return Ontology.add ( new TDLAxiomConceptInclusion ( C, D ) ); }
		/// axiom C1 = ... = Cn
	TDLAxiom* equalConcepts ( void )
		{ return Ontology.add ( new TDLAxiomEquivalentConcepts(getExpressionManager()->getArgList()) ); }
		/// axiom C1 != ... != Cn
	TDLAxiom* disjointConcepts ( void )
		{ return Ontology.add ( new TDLAxiomDisjointConcepts(getExpressionManager()->getArgList()) ); }


	// Role axioms

		/// R = Inverse(S)
	TDLAxiom* setInverseRoles ( TORoleExpr* R, TORoleExpr* S )
		{ return Ontology.add ( new TDLAxiomRoleInverse(R,S) ); }
		/// axiom (R [= S)
	TDLAxiom* impliesORoles ( TORoleComplexExpr* R, TORoleExpr* S )
		{ return Ontology.add ( new TDLAxiomORoleSubsumption ( R, S ) ); }
		/// axiom (R [= S)
	TDLAxiom* impliesDRoles ( TDRoleExpr* R, TDRoleExpr* S )
		{ return Ontology.add ( new TDLAxiomDRoleSubsumption ( R, S ) ); }
		/// axiom R1 = R2 = ...
	TDLAxiom* equalORoles ( void )
		{ return Ontology.add ( new TDLAxiomEquivalentORoles(getExpressionManager()->getArgList()) ); }
		/// axiom R1 = R2 = ...
	TDLAxiom* equalDRoles ( void )
		{ return Ontology.add ( new TDLAxiomEquivalentDRoles(getExpressionManager()->getArgList()) ); }
		/// axiom R1 != R2 != ...
	TDLAxiom* disjointORoles ( void )
		{ return Ontology.add ( new TDLAxiomDisjointORoles(getExpressionManager()->getArgList()) ); }
		/// axiom R1 != R2 != ...
	TDLAxiom* disjointDRoles ( void )
		{ return Ontology.add ( new TDLAxiomDisjointDRoles(getExpressionManager()->getArgList()) ); }

		/// Domain (R C)
	TDLAxiom* setODomain ( TORoleExpr* R, TConceptExpr* C )
		{ return Ontology.add ( new TDLAxiomORoleDomain ( R, C ) ); }
		/// Domain (R C)
	TDLAxiom* setDDomain ( TDRoleExpr* R, TConceptExpr* C )
		{ return Ontology.add ( new TDLAxiomDRoleDomain ( R, C ) ); }
		/// Range (R C)
	TDLAxiom* setORange ( TORoleExpr* R, TConceptExpr* C )
		{ return Ontology.add ( new TDLAxiomORoleRange ( R, C ) ); }
		/// Range (R E)
	TDLAxiom* setDRange ( TDRoleExpr* R, TDataExpr* E )
		{ return Ontology.add ( new TDLAxiomDRoleRange ( R, E ) ); }

		/// Transitive (R)
	TDLAxiom* setTransitive ( TORoleExpr* R )
		{ return Ontology.add ( new TDLAxiomRoleTransitive(R) ); }
		/// Reflexive (R)
	TDLAxiom* setReflexive ( TORoleExpr* R )
		{ return Ontology.add ( new TDLAxiomRoleReflexive(R) ); }
		/// Irreflexive (R): Domain(R) = \neg ER.Self
	TDLAxiom* setIrreflexive ( TORoleExpr* R )
		{ return Ontology.add ( new TDLAxiomRoleIrreflexive(R) ); }
		/// Symmetric (R): R [= R^-
	TDLAxiom* setSymmetric ( TORoleExpr* R )
		{ return Ontology.add ( new TDLAxiomRoleSymmetric(R) ); }
		/// Asymmetric (R): disjoint(R,R^-)
	TDLAxiom* setAsymmetric ( TORoleExpr* R )
		{ return Ontology.add ( new TDLAxiomRoleAsymmetric(R) ); }
		/// Functional (R)
	TDLAxiom* setOFunctional ( TORoleExpr* R )
		{ return Ontology.add ( new TDLAxiomORoleFunctional(R) ); }
		/// Functional (R)
	TDLAxiom* setDFunctional ( TDRoleExpr* R )
		{ return Ontology.add ( new TDLAxiomDRoleFunctional(R) ); }
		/// InverseFunctional (R)
	TDLAxiom* setInverseFunctional ( TORoleExpr* R )
		{ return Ontology.add ( new TDLAxiomRoleInverseFunctional(R) ); }


	// Individual axioms

		/// axiom I e C
	TDLAxiom* instanceOf ( TIndividualExpr* I, TConceptExpr* C )
		{ return Ontology.add ( new TDLAxiomInstanceOf(I,C) ); }
		/// axiom <I,J>:R
	TDLAxiom* relatedTo ( TIndividualExpr* I, TORoleExpr* R, TIndividualExpr* J )
		{ return Ontology.add ( new TDLAxiomRelatedTo(I,R,J) ); }
		/// axiom <I,J>:\neg R
	TDLAxiom* relatedToNot ( TIndividualExpr* I, TORoleExpr* R, TIndividualExpr* J )
		{ return Ontology.add ( new TDLAxiomRelatedToNot(I,R,J) ); }
		/// axiom (value I A V)
	TDLAxiom* valueOf ( TIndividualExpr* I, TDRoleExpr* A, TDataValueExpr* V )
		{ return Ontology.add ( new TDLAxiomValueOf(I,A,V) ); }
		/// axiom <I,V>:\neg A
	TDLAxiom* valueOfNot ( TIndividualExpr* I, TDRoleExpr* A, TDataValueExpr* V )
		{ return Ontology.add ( new TDLAxiomValueOfNot(I,A,V) ); }
		/// same individuals
	TDLAxiom* processSame ( void )
		{ return Ontology.add ( new TDLAxiomSameIndividuals(getExpressionManager()->getArgList()) ); }
		/// different individuals
	TDLAxiom* processDifferent ( void )
		{ return Ontology.add ( new TDLAxiomDifferentIndividuals(getExpressionManager()->getArgList()) ); }
		/// let all concept expressions in the ArgQueue to be fairness constraints
	TDLAxiom* setFairnessConstraint ( void )
		{ return Ontology.add ( new TDLAxiomFairnessConstraint(getExpressionManager()->getArgList()) ); }

		/// retract an axiom
	void retract ( TDLAxiom* axiom ) { Ontology.retract(axiom); }

	//******************************************
	//* ASK part
	//******************************************

	/*
	 * Before execution of any query the Kernel make sure that the KB is in an appropriate
	 * state: Preprocessed, Classified or Realised. If the ontology was changed between asks,
	 * incremental classification is performed and the corrected result is returned.
	 */

		/// return consistency status of KB
	bool isKBConsistent ( void )
	{
		if ( getStatus() <= kbLoading )
			processKB(kbCChecked);
		return getTBox()->isConsistent();
	}
		/// ensure that KB is preprocessed/consistence checked
	void preprocessKB ( void )
	{
		if ( !isKBConsistent() )
			throw EFPPInconsistentKB();
	}
		/// ensure that KB is classified
	void classifyKB ( void )
	{
		if ( !isKBClassified() )
			processKB(kbClassified);
		if ( !isKBConsistent() )
			throw EFPPInconsistentKB();
	}
		/// ensure that KB is realised
	void realiseKB ( void )
	{
		if ( !isKBRealised() )
			processKB(kbRealised);
		if ( !isKBConsistent() )
			throw EFPPInconsistentKB();
	}
		/// try to perform the incremental reasoning on the changed ontology
	bool tryIncremental ( void );
		/// force the re-classification of the changed ontology
	void forceReload ( void );

	// role info retrieval

		/// @return true iff object role is functional
	bool isFunctional ( const TORoleExpr* R )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		if ( isUniversalRole(R) )
			return false;	// universal role is not functional
		if ( isEmptyRole(R) )
			return true;	// empty role is functional

		return getFunctionality ( getRole ( R, "Role expression expected in isFunctional()" ) );
	}
		/// @return true iff data role is functional
	bool isFunctional ( const TDRoleExpr* R )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		if ( isUniversalRole(R) )
			return false;	// universal role is not functional
		if ( isEmptyRole(R) )
			return true;	// empty role is functional

		return getFunctionality ( getRole ( R, "Role expression expected in isFunctional()" ) );
	}
		/// @return true iff role is inverse-functional
	bool isInverseFunctional ( const TORoleExpr* R )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		if ( isUniversalRole(R) )
			return false;	// universal role is not functional
		if ( isEmptyRole(R) )
			return true;	// empty role is functional

		return getFunctionality ( getRole ( R, "Role expression expected in isInverseFunctional()" )->inverse() );
	}
		/// @return true iff role is transitive
	bool isTransitive ( const TORoleExpr* R )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		if ( isUniversalRole(R) )
			return true;	// universal role is transitive
		if ( isEmptyRole(R) )
			return true;	// empty role is transitive

		TRole* r = getRole ( R, "Role expression expected in isTransitive()" );
		if ( !r->isTransitivityKnown() )	// calculate transitivity
			r->setTransitive(checkTransitivity(e(R)));
		return r->isTransitive();
	}
		/// @return true iff role is symmetric
	bool isSymmetric ( const TORoleExpr* R )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		if ( isUniversalRole(R) )
			return true;	// universal role is symmetric
		if ( isEmptyRole(R) )
			return true;	// empty role is symmetric

		TRole* r = getRole ( R, "Role expression expected in isSymmetric()" );
		if ( !r->isSymmetryKnown() )	// calculate symmetry
			r->setSymmetric(checkSymmetry(e(R)));
		return r->isSymmetric();
	}
		/// @return true iff role is asymmetric
	bool isAsymmetric ( const TORoleExpr* R )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		if ( isUniversalRole(R) )
			return false;	// universal role is not asymmetric
		if ( isEmptyRole(R) )
			return true;	// empty role is symmetric

		TRole* r = getRole ( R, "Role expression expected in isAsymmetric()" );
		if ( !r->isAsymmetryKnown() )	// calculate asymmetry
			r->setAsymmetric(getTBox()->isDisjointRoles(r,r->inverse()));
		return r->isAsymmetric();
	}
		/// @return true iff role is reflexive
	bool isReflexive ( const TORoleExpr* R )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		if ( isUniversalRole(R) )
			return true;	// universal role is reflexive
		if ( isEmptyRole(R) )
			return false;	// empty role is not reflexive

		TRole* r = getRole ( R, "Role expression expected in isReflexive()" );
		if ( !r->isReflexivityKnown() )	// calculate reflexivity
			r->setReflexive(checkReflexivity(e(R)));
		return r->isReflexive();
	}
		/// @return true iff role is irreflexive
	bool isIrreflexive ( const TORoleExpr* R )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		if ( isUniversalRole(R) )
			return false;	// universal role is not irreflexive
		if ( isEmptyRole(R) )
			return true;	// empty role is irreflexive

		TRole* r = getRole ( R, "Role expression expected in isIrreflexive()" );
		if ( !r->isIrreflexivityKnown() )	// calculate irreflexivity
			r->setIrreflexive(getTBox()->isIrreflexive(r));
		return r->isIrreflexive();
	}
		/// @return true if R is a sub-role of S
	bool isSubRoles ( const TORoleExpr* R, const TORoleExpr* S )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		if ( isEmptyRole(R) || isUniversalRole(S) )
			return true;	// \bot [= X [= \top
		if ( isUniversalRole(R) && isEmptyRole(S) )
			return false;	// as \top [= \bot leads to inconsistent ontology

		// told case first
		if ( *getRole ( R, "Role expression expected in isSubRoles()" ) <=
			 *getRole ( S, "Role expression expected in isSubRoles()" ) )
			return true;
		// check the general case
		// FIXME!! cache it later
		DLTree* r = e(R), *s = e(S);
		return checkRoleSubsumption ( r, s );
	}
		/// @return true iff two roles are disjoint
	bool isDisjointRoles ( const TORoleExpr* R, const TORoleExpr* S )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		if ( isUniversalRole(R) || isUniversalRole(S) )
			return false;	// universal role is not disjoint with anything
		if ( isEmptyRole(R) || isEmptyRole(S) )
			return true;	// empty role is disjoint with everything
		return getTBox()->isDisjointRoles (
			getRole ( R, "Role expression expected in isDisjointRoles()" ),
			getRole ( S, "Role expression expected in isDisjointRoles()" ) );
	}
		/// @return true iff two roles are disjoint
	bool isDisjointRoles ( const TDRoleExpr* R, const TDRoleExpr* S )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		if ( isUniversalRole(R) || isUniversalRole(S) )
			return false;	// universal role is not disjoint with anything
		if ( isEmptyRole(R) || isEmptyRole(S) )
			return true;	// empty role is disjoint with everything
		return getTBox()->isDisjointRoles (
			getRole ( R, "Role expression expected in isDisjointRoles()" ),
			getRole ( S, "Role expression expected in isDisjointRoles()" ) );
	}
		/// @return true if R is a super-role of a chain holding in the args
	bool isSubChain ( const TORoleExpr* R )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		if ( isUniversalRole(R) )
			return true;	// universal role is a super of any chain
		if ( isEmptyRole(R) )
			return false;	// empty role is not a super of any chain
		return checkSubChain ( getRole ( R, "Role expression expected in isSubChain()" ) );
	}

	// single satisfiability

		/// @return true iff C is satisfiable
	bool isSatisfiable ( const TConceptExpr* C ) { preprocessKB(); return checkSat(e(C)); }
		/// @return true iff C [= D holds
	bool isSubsumedBy ( const TConceptExpr* C, const TConceptExpr* D ) { preprocessKB(); return checkSub ( e(C), e(D) ); }
		/// @return true iff C is disjoint with D; that is, C [= \not D holds
	bool isDisjoint ( const TConceptExpr* C, const TConceptExpr* D ) { preprocessKB(); return checkSub ( e(C), createSNFNot(e(D)) ); }
		/// @return true iff C is equivalent to D
	bool isEquivalent ( const TConceptExpr* C, const TConceptExpr* D )
		{ preprocessKB(); return isSubsumedBy ( C, D ) && isSubsumedBy ( D, C ); }

	// concept hierarchy

		/// apply actor::apply() to all DIRECT super-concepts of [complex] C
	template<class Actor>
	void getSupConcepts ( const TConceptExpr* C, bool direct, Actor& actor )
	{
		classifyKB();	// ensure KB is ready to answer the query
		setUpCache ( e(C), csClassified );
		Taxonomy* tax = getCTaxonomy();
		if ( direct )
			tax->getRelativesInfo</*needCurrent=*/false, /*onlyDirect=*/true, /*upDirection=*/true> ( cachedVertex, actor );
		else
			tax->getRelativesInfo</*needCurrent=*/false, /*onlyDirect=*/false, /*upDirection=*/true> ( cachedVertex, actor );
	}
		/// apply actor::apply() to all DIRECT sub-concepts of [complex] C
	template<class Actor>
	void getSubConcepts ( const TConceptExpr* C, bool direct, Actor& actor )
	{
		classifyKB();	// ensure KB is ready to answer the query
		setUpCache ( e(C), csClassified );
		Taxonomy* tax = getCTaxonomy();
		if ( direct )
			tax->getRelativesInfo</*needCurrent=*/false, /*onlyDirect=*/true, /*upDirection=*/false> ( cachedVertex, actor );
		else
			tax->getRelativesInfo</*needCurrent=*/false, /*onlyDirect=*/false, /*upDirection=*/false> ( cachedVertex, actor );
	}
		/// apply actor::apply() to all synonyms of [complex] C
	template<class Actor>
	void getEquivalentConcepts ( const TConceptExpr* C, Actor& actor )
	{
		classifyKB();	// ensure KB is ready to answer the query
		setUpCache ( e(C), csClassified );
		actor.apply(*cachedVertex);
	}

	// role hierarchy

		/// apply actor::apply() to all DIRECT super-roles of [complex] R
	template<class Actor>
	void getSupRoles ( const TRoleExpr* r, bool direct, Actor& actor )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		TRole* R = getRole ( r, "Role expression expected in getSupRoles()" );
		Taxonomy* tax = getTaxonomy(R);
		if ( direct )
			tax->getRelativesInfo</*needCurrent=*/false, /*onlyDirect=*/true, /*upDirection=*/true> ( getTaxVertex(R), actor );
		else
			tax->getRelativesInfo</*needCurrent=*/false, /*onlyDirect=*/false, /*upDirection=*/true> ( getTaxVertex(R), actor );
	}
		/// apply actor::apply() to all DIRECT sub-roles of [complex] R
	template<class Actor>
	void getSubRoles ( const TRoleExpr* r, bool direct, Actor& actor )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		TRole* R = getRole ( r, "Role expression expected in getSubRoles()" );
		Taxonomy* tax = getTaxonomy(R);
		if ( direct )
			tax->getRelativesInfo</*needCurrent=*/false, /*onlyDirect=*/true, /*upDirection=*/false> ( getTaxVertex(R), actor );
		else
			tax->getRelativesInfo</*needCurrent=*/false, /*onlyDirect=*/false, /*upDirection=*/false> ( getTaxVertex(R), actor );
	}
		/// apply actor::apply() to all synonyms of [complex] R
	template<class Actor>
	void getEquivalentRoles ( const TRoleExpr* r, Actor& actor )
	{
		preprocessKB();	// ensure KB is ready to answer the query
		TRole* R = getRole ( r, "Role expression expected in getEquivalentRoles()" );
		actor.apply(*getTaxVertex(R));
	}

	// domain and range as a set of named concepts

		/// apply actor::apply() to all DIRECT NC that are in the domain of [complex] R
	template<class Actor>
	void getRoleDomain ( const TRoleExpr* r, bool direct, Actor& actor )
	{
		classifyKB();	// ensure KB is ready to answer the query
		setUpCache ( createSNFExists ( e(r), new DLTree(TOP) ), csClassified );
		Taxonomy* tax = getCTaxonomy();
		if ( direct )	// gets an exact domain is named concept; otherwise, set of the most specific concepts
			tax->getRelativesInfo</*needCurrent=*/true, /*onlyDirect=*/true, /*upDirection=*/true> ( cachedVertex, actor );
		else			// gets all named classes that are in the domain of a role
			tax->getRelativesInfo</*needCurrent=*/true, /*onlyDirect=*/false, /*upDirection=*/true> ( cachedVertex, actor );
	}
		/// apply actor::apply() to all DIRECT NC that are in the range of [complex] R
	template<class Actor>
	void getRoleRange ( const TORoleExpr* r, bool direct, Actor& actor )
		{ getRoleDomain ( getExpressionManager()->Inverse(r), direct, actor ); }

	// instances

		/// apply actor::apply() to all direct instances of given [complex] C
	template<class Actor>
	void getDirectInstances ( const TConceptExpr* C, Actor& actor )
	{
		realiseKB();	// ensure KB is ready to answer the query
		setUpCache ( e(C), csClassified );

		// implement 1-level check by hand

		// if the root vertex contains individuals -- we are done
		if ( actor.apply(*cachedVertex) )
			return;

		// if not, just go 1 level down and apply the actor regardless of what's found
		// FIXME!! check again after bucket-method will be implemented
		for ( TaxonomyVertex::iterator p = cachedVertex->begin(/*upDirection=*/false),
				p_end = cachedVertex->end(/*upDirection=*/false); p < p_end; ++p )
			actor.apply(**p);
	}

		/// apply actor::apply() to all instances of given [complex] C
	template<class Actor>
	void getInstances ( const TConceptExpr* C, Actor& actor )
	{	// FIXME!! check for Racer's/IS approach
		realiseKB();	// ensure KB is ready to answer the query
		setUpCache ( e(C), csClassified );
		Taxonomy* tax = getCTaxonomy();
		tax->getRelativesInfo</*needCurrent=*/true, /*onlyDirect=*/false, /*upDirection=*/false> ( cachedVertex, actor );
	}

		/// apply actor::apply() to all DIRECT concepts that are types of an individual I
	template<class Actor>
	void getTypes ( const TIndividualExpr* I, bool direct, Actor& actor )
	{
		realiseKB();	// ensure KB is ready to answer the query
		getSupConcepts ( getExpressionManager()->OneOf(I), direct, actor );
	}
		/// apply actor::apply() to all synonyms of an individual I
	template<class Actor>
	void getSameAs ( const TIndividualExpr* I, Actor& actor )
	{
		realiseKB();	// ensure KB is ready to answer the query
		getEquivalentConcepts ( getExpressionManager()->OneOf(I), actor );
	}
		/// @return true iff I and J refer to the same individual
	bool isSameIndividuals ( const TIndividualExpr* I, const TIndividualExpr* J )
	{
		realiseKB();
		TIndividual* i = getIndividual ( I, "Only known individuals are allowed in the isSameAs()" );
		TIndividual* j = getIndividual ( J, "Only known individuals are allowed in the isSameAs()" );
		return getTBox()->isSameIndividuals(i,j);
	}
		/// @return true iff individual I is instance of given [complex] C
	bool isInstance ( const TIndividualExpr* I, const TConceptExpr* C )
	{
		realiseKB();	// ensure KB is ready to answer the query
		getIndividual ( I, "individual name expected in the isInstance()" );
		return isSubsumedBy ( getExpressionManager()->OneOf(I), C );
	}
		/// @return in Rs all (DATA)-roles R s.t. (I,x):R; add inverses if NEEDI is true
	void getRelatedRoles ( const TIndividualExpr* I, NamesVector& Rs, bool data, bool needI );
		/// set RESULT into set of J's such that R(I,J)
	void getRoleFillers ( const TIndividualExpr* I, const TORoleExpr* R, IndividualSet& Result );
		/// set RESULT into set of J's such that R(I,J)
	bool isRelated ( const TIndividualExpr* I, const TORoleExpr* R, const TIndividualExpr* J );
}; // ReasoningKernel

#endif
