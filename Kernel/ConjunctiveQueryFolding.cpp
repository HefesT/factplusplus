/* This file is part of the FaCT++ DL reasoner
Copyright (C) 2011-2013 by Stanislav Kikot, Dmitry Tsarkov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <iostream>
#include <iomanip>

#include "Kernel.h"
#include "tExpressionPrinterLISP.h"
#include "QR.h"

//-------------------------------------------------------------
// Expression typedefs
//-------------------------------------------------------------

	/// general expression
typedef ReasoningKernel::TExpr TExpr;
	/// concept expression
typedef ReasoningKernel::TConceptExpr TConceptExpr;
	/// individual expression
typedef ReasoningKernel::TIndividualExpr TIndividualExpr;
	/// role expression
typedef ReasoningKernel::TRoleExpr TRoleExpr;
	/// object role complex expression (including role chains and projections)
typedef ReasoningKernel::TORoleComplexExpr TORoleComplexExpr;
	/// object role expression
typedef ReasoningKernel::TORoleExpr TORoleExpr;
	/// data role expression
typedef ReasoningKernel::TDRoleExpr TDRoleExpr;
	/// data expression
typedef ReasoningKernel::TDataExpr TDataExpr;
	/// data type expression
typedef ReasoningKernel::TDataTypeExpr TDataTypeExpr;
	/// data value expression
typedef ReasoningKernel::TDataValueExpr TDataValueExpr;
	/// data facet expression
typedef const TDLFacetExpression TFacetExpr;

TExpressionManager* pEM;
VariableFactory VarFact;

#define C(name) TConceptExpr* name = pEM->Concept(#name)
#define R(name)	TORoleExpr* name = pEM->ObjectRole(#name)
#define D(c) pEM->Concept(#c)
#define E(r) pEM->Exists(pEM->ObjectRole(#r),pEM->Top())
#define Ei(r) pEM->Exists(pEM->Inverse(pEM->ObjectRole(#r)),pEM->Top())

//----------------------------------------------------------------------------------
// some queries
//----------------------------------------------------------------------------------

void buildQueryFigure2 ( QRQuery * query)
{
	const QRVariable* x = VarFact.getNewVar("x");
	const QRVariable* y = VarFact.getNewVar("y");
	const QRVariable* z = VarFact.getNewVar("z");
	const QRVariable* w = VarFact.getNewVar("v");
	query -> setVarFree(x);
	query -> setVarFree(y);

 	R(R1);
	R(R2);
	R(R3);
	R(R4);
	R(R5);
	R(R6);

	query -> addAtom(new QRRoleAtom(R1,x,z));
	query -> addAtom(new QRRoleAtom(R2,x,w));
	query -> addAtom(new QRRoleAtom(R3,z,y));
	query -> addAtom(new QRRoleAtom(R4,y,w));
	query -> addAtom(new QRRoleAtom(R5,z,w));
	query -> addAtom(new QRRoleAtom(R6,y,y));
}

void buildSimpleQuery ( QRQuery * query)
{
	const QRVariable* x = VarFact.getNewVar("x");
	const QRVariable* y = VarFact.getNewVar("y");
	query -> setVarFree(x);
	query -> setVarFree(y);

	R(R1);
	R(R2);

	query -> addAtom(new QRRoleAtom(R1,x,y));
	query -> addAtom(new QRRoleAtom(R2,y,x));
}

void buildVerySimpleQuery ( QRQuery * query)
{
	const QRVariable* x = VarFact.getNewVar("x");
	query -> setVarFree(x);

	R(R1);

	query -> addAtom(new QRRoleAtom(R1,x,x));
}

void buildVerySimpleQueryLUBM1 ( QRQuery * query)
{
	const QRVariable* x = VarFact.getNewVar("x");
	query -> setVarFree(x);
	const QRVariable* y = VarFact.getNewVar("y");
	query -> setVarFree(y);


	R(R1);
	C(C1);

	query -> addAtom(new QRRoleAtom(R1,x,y));
	query -> addAtom(new QRConceptAtom(C1,x));
}

void buildLUBM2Query(int n, QRQuery * query) {
	switch(n) {
		case 1:
			const QRVariable* v0 = VarFact.getNewVar("v0");
			const QRVariable* v1 = VarFact.getNewVar("v1");
			const QRVariable* v2 = VarFact.getNewVar("v2");
			const QRVariable* v3 = VarFact.getNewVar("v3");
			query -> setVarFree(v0);
			query -> setVarFree(v2);

			C(Student);
			C(Course);
			C(Faculty);
			C(Department);
			R(takesCourse);
			R(teacherOf);
			R(worksFor);
			R(memberOf);
			query -> addAtom(new QRConceptAtom(Student,v0));
			query -> addAtom(new QRConceptAtom(Course,v1));
			query -> addAtom(new QRConceptAtom(Faculty,v2));
			query -> addAtom(new QRConceptAtom(Department,v3));
			query -> addAtom(new QRRoleAtom(takesCourse,v0,v1));
			query -> addAtom(new QRRoleAtom(teacherOf,v2,v1));
			query -> addAtom(new QRRoleAtom(worksFor,v2,v3));
			query -> addAtom(new QRRoleAtom(memberOf,v0,v3));
			break;
	}
}

inline QRQuery* createQuery(void)
{
	QRQuery* query = new QRQuery();
	buildLUBM2Query(1, query);
	return query;
}

//------------------------------------------------------
// QRVarSet support
//------------------------------------------------------

typedef std::set<const QRVariable*> QRVarSet;

//----------------------------------------------------------------------------------
// print methods
//----------------------------------------------------------------------------------

static inline std::ostream&
operator << ( std::ostream& o, const QRVariable* var )
{
	o << var->getName().c_str();
	return o;
}

static inline std::ostream&
operator << ( std::ostream& o, const QRAtom* atom )
{
	if ( atom == NULL )
		o << "NULL";
	else if ( const QRRoleAtom* role = dynamic_cast<const QRRoleAtom*>(atom) )
		o << dynamic_cast<const TDLObjectRoleName*>(role->getRole())->getName() << "("
		  << dynamic_cast<const QRVariable*>(role->getArg1()) << ","
		  << dynamic_cast<const QRVariable*>(role->getArg2()) << ")";
	else if ( const QRConceptAtom* concept = dynamic_cast<const QRConceptAtom*>(atom) )
		o << dynamic_cast<const TDLConceptName*>(concept->getConcept())->getName() << "("
		  << dynamic_cast<const QRVariable*>(concept->getArg()) << ")";
	return o;
}

static inline std::ostream&
operator << ( std::ostream& o, const QRQuery * query )
{
	o << "Query = {";
	for (QRSetAtoms::const_iterator p = query->Body.begin(), p_end = query->Body.end(); p != p_end; ++p )
		o << "\n" << *p;
	o << " }\n";
	return o;
}

//----------------------------------------------------------------------------------
// smart AND method
//----------------------------------------------------------------------------------

static inline TConceptExpr*
And ( TConceptExpr* C, TConceptExpr* D )
{
	if ( C == D )
		return C;
	if ( dynamic_cast<const TDLConceptTop*>(C) )
		return D;
	if ( dynamic_cast<const TDLConceptTop*>(D) )
		return C;
	return pEM->And(C,D);
}

class QueryConnectednessChecker {
	QRVarSet PassedVertice;
	QRQuery * Query; //const
public:

	QueryConnectednessChecker(QRQuery * query) : PassedVertice(), Query(query) {} ;

	bool isConnected() {
		QRSetAtoms::const_iterator atomIterator = Query -> Body.begin();


		if (const QRRoleAtom * atom = dynamic_cast<const QRRoleAtom*> (* atomIterator))  {
			const QRVariable * arg1 = dynamic_cast <const QRVariable *> (atom -> getArg1() ) ;
			MarkVertex(arg1);
		} else if (const QRConceptAtom * atom = dynamic_cast<const QRConceptAtom*> (* atomIterator))  {
			const QRVariable * arg = dynamic_cast <const QRVariable *> (atom -> getArg() ) ;
			MarkVertex(arg);
		} else {
			throw EFaCTPlusPlus ("Unsupported atom in query rewriting");
		}

		for (atomIterator = Query->Body.begin();
		 atomIterator != Query->Body.end();
		 ++atomIterator )
		{
			if (const QRRoleAtom * atom = dynamic_cast<const QRRoleAtom*> (* atomIterator) ) {
				const QRVariable * arg1 = dynamic_cast <const QRVariable *> (atom -> getArg1() ) ;
				const QRVariable * arg2 = dynamic_cast <const QRVariable *> (atom -> getArg2() ) ;
				if (
					PassedVertice.find( arg1 ) == PassedVertice.end()
					||
					PassedVertice.find( arg2 ) == PassedVertice.end()
					)
				return false;
			} else if  (const QRConceptAtom * atom = dynamic_cast<const QRConceptAtom*> (* atomIterator) ) {
				if (PassedVertice.find( dynamic_cast <const QRVariable *> (atom -> getArg()) ) == PassedVertice.end() )
					return false;
			} else {
				throw EFaCTPlusPlus("Unsupported atom in query rewriting");
			}
		}
		return true;
	}

	void MarkVertex( const QRVariable * var) {

		PassedVertice.insert(var);

		for (QRSetAtoms::const_iterator atomIterator = Query->Body.begin();
		 atomIterator != Query->Body.end();
		 ++atomIterator )
		{
			if (const QRRoleAtom * atom = dynamic_cast<const QRRoleAtom*> (* atomIterator)) {
				const QRVariable * arg1 = dynamic_cast <const QRVariable *> (atom -> getArg1() ) ;
				const QRVariable * arg2 = dynamic_cast <const QRVariable *> (atom -> getArg2() ) ;

				if (arg1 != var && arg2 != var || arg1 == var && arg2 == var) {
					continue;
				}

				const QRVariable * neighbour;
				if (arg1 == var ) {
					neighbour = arg2;
				} else {
					neighbour = arg1;
				}

				if (PassedVertice.find( neighbour ) != PassedVertice.end()) {
					continue;
				}

				MarkVertex(neighbour);
			}
		}
	}
};


bool PossiblyReplaceAtom(QRQuery* query,
		QRSetAtoms::const_iterator atomIterator, QRAtom* newAtom, const QRVariable* newArg,
		std::set<const QRAtom*>& passedAtoms)
{
	std::cout << "Modified code starts here!\nBefore replacing in copy.\n" << query;
	QRAtom* oldAtom = query->Body.replaceAtom(atomIterator, newAtom);
	query->setVarFree(newArg);
	std::cout << "Running Checker" << std::endl;
	QueryConnectednessChecker checker(query);
	bool ret;
	if (checker.isConnected())
	{
		std::cout << "Connected\nAfter replacing in Query\n" << query;
		ret = true;
	}
	else
	{
		std::cout << "Disconnected" << std::endl;
		// restore the old query
		newAtom = oldAtom;
		oldAtom = query->Body.replaceAtom(atomIterator, oldAtom);
		query->FreeVars.erase(newArg);
		ret = false;
	}
	passedAtoms.insert(newAtom);
	delete oldAtom;
	return ret;
}

/// map between new vars and original vars
std::map<const QRVariable*, const QRVariable*> NewVarMap;

/// init vars map
static inline void initVarMap ( const QRQuery* query )
{
	NewVarMap.clear();
	for ( std::set<const QRVariable*>::const_iterator p = query->FreeVars.begin(), p_end = query->FreeVars.end(); p != p_end; ++p )
		NewVarMap[*p] = *p;
}

/// create a new var which is a copy of an existing one
static inline const QRVariable*
getNewCopyVar ( const QRVariable* old, int suffix )
{
	char buf[40];
	sprintf(buf, "_%d", suffix);
	const QRVariable* var = VarFact.getNewVar(old->getName()+buf);
	NewVarMap[var] = old;
	return var;
}


void transformQueryPhase1(QRQuery * query) {
	std::set<const QRAtom*> passedAtoms;
	int n = 0;
	QRSetAtoms::const_iterator nextAtomIterator;

	// clear the map and make identities
	initVarMap(query);

	std::cout << "Phase 1 starts" << std::endl;

	for (QRSetAtoms::const_iterator i = query->Body.begin(), p_end = query->Body.end(); i != p_end; )
	{
		const QRRoleAtom* atom = dynamic_cast<const QRRoleAtom*>(*i);
		// atom is a new role atom
		if ( atom == NULL || passedAtoms.count(atom) > 0 )
		{
			i++;
			continue;
		}

		const TDLObjectRoleExpression* role = atom -> getRole();
		const QRVariable* arg1 = dynamic_cast <const QRVariable*>(atom->getArg1()) ;
		const QRVariable* arg2 = dynamic_cast <const QRVariable*>(atom->getArg2()) ;

		if ( query->isFreeVar(arg2) )
		{
			const QRVariable* newArg = getNewCopyVar ( arg2, ++n );
			QRAtom* newAtom = new QRRoleAtom(role, arg1, newArg);
			if ( PossiblyReplaceAtom(query, i, newAtom, newArg, passedAtoms) )
				continue;
		}
		else if ( query->isFreeVar(arg1) )
		{
			const QRVariable* newArg = getNewCopyVar ( arg1, ++n );
			QRAtom* newAtom = new QRRoleAtom(role, newArg, arg2);
			if ( PossiblyReplaceAtom(query, i, newAtom, newArg, passedAtoms) )
				continue;
		}
		i++;
	}
}

class TermAssigner {
	QRVarSet PassedVertice;
	QRQuery * Query;
	int N;

protected:
	TConceptExpr* createVar ( const QRVariable* v )
	{
		if ( Query->isFreeVar(NewVarMap[v]) )
		{
			char buf[40];
			sprintf(buf, ":%d", ++N);
			return pEM->Concept(NewVarMap[v]->getName()+buf);
		}

		return pEM->Top();
	}

public:
	TermAssigner(QRQuery * query) : Query (query), N(0) {}

	TConceptExpr* Assign(QRAtom* previousAtom, const QRVariable* v) {
		std::cout << "Assign:\n variable: " << v << "\n atom: " << previousAtom << std::endl;
		PassedVertice.insert(v);

		TConceptExpr* s = pEM->Top(), *t = createVar(v);

		for (QRSetAtoms::const_iterator atomIterator = Query->Body.begin();
			 atomIterator != Query->Body.end();
			 ++atomIterator)
		{

			if (const QRRoleAtom * atom = dynamic_cast<const QRRoleAtom*> (* atomIterator)) {
				const TDLObjectRoleExpression* role = atom -> getRole();
				const QRVariable * arg1 = dynamic_cast <const QRVariable *> (atom -> getArg1() ) ;
				const QRVariable * arg2 = dynamic_cast <const QRVariable *> (atom -> getArg2() ) ;

				if (*atomIterator == previousAtom) continue;

				if (arg1 == v) {
					TConceptExpr * p = Assign ( *atomIterator, arg2);
					p = pEM->Exists(role, p);
					s = And(s, p);
				}

				if (arg2 == v) {
					TConceptExpr * p = Assign ( *atomIterator, arg1);
					p = pEM->Exists(pEM -> Inverse(role), p);
					s = And(s, p);
				}
			}

			if (const QRConceptAtom * atom = dynamic_cast<const QRConceptAtom*> (* atomIterator)) {
				const TDLConceptExpression* concept = atom -> getConcept();
				const QRVariable * arg = dynamic_cast <const QRVariable *> (atom -> getArg() ) ;
				if (arg == v) {
					s = And(s, concept);
				}
			}
		}

		return And(t, s);
	}
	void DeleteFictiveVariables () {
		QRQuery::QRVarSet RealFreeVars;
		for (QRSetAtoms::const_iterator atomIterator = Query->Body.begin();
				 atomIterator != Query->Body.end();
				 ++atomIterator)
		{
			if (const QRRoleAtom * atom = dynamic_cast<const QRRoleAtom*> (* atomIterator) ){
				const QRVariable * arg1 = dynamic_cast <const QRVariable *> (atom -> getArg1() ) ;
				const QRVariable * arg2 = dynamic_cast <const QRVariable *> (atom -> getArg2() ) ;
				if ( Query->isFreeVar(arg1) )
					RealFreeVars.insert(arg1);
				if ( Query->isFreeVar(arg2) )
					RealFreeVars.insert(arg2);
			}
		}
		Query->FreeVars = RealFreeVars;
	}
};


TConceptExpr * transformQueryPhase2(QRQuery * query) {
	TermAssigner assigner (query);
	assigner.DeleteFictiveVariables();
	const QRVariable* var = *(query->FreeVars.begin());
	std::cout << "Assigner initialised; var: " << var << "\n";
	return assigner.Assign ( NULL, var );
};

bool IsNominal(const TConceptExpr * expr ) {
	if (const TDLConceptName * conceptName = dynamic_cast<const TDLConceptName * >(expr)) {
		if ((conceptName -> getName())[0] >='a' && (conceptName -> getName())[0] <= 'z') {
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

class TDepthMeasurer: public DLExpressionVisitorEmpty
{
protected:	// members
	std::map<TConceptExpr *, int> DepthOfNominalOccurences;
	int CurrentDepth;
	int TotalNominalOccurences;

public:		// interface
		/// init c'tor
	TDepthMeasurer ( ) : DepthOfNominalOccurences(), CurrentDepth(0), TotalNominalOccurences(0)  {}
		/// empty d'tor
	virtual ~TDepthMeasurer ( void ) {}

public:		// visitor interface
	// concept expressions



	virtual void visit ( const TDLConceptTop& expr ATTR_UNUSED ) {
	}

	virtual void visit ( const TDLConceptName& expr )
	{
		if (IsNominal(& expr)) {
			DepthOfNominalOccurences.insert(std::pair<const TConceptExpr*, int>( dynamic_cast<const TConceptExpr*> (&(expr)) , CurrentDepth));
			++TotalNominalOccurences;
		}
	}

	virtual void visit ( const TDLConceptAnd& expr ) {
		for ( TDLNAryExpression<TConceptExpr>::iterator p = expr.begin(), p_end = expr.end(); p != p_end; ++p ) {
			(*p)->accept(*this);
		}
	}


	virtual void visit ( const TDLConceptObjectExists& expr ) {
		const TDLObjectRoleExpression* role1 = expr.getOR();
		if ( dynamic_cast<const  TDLObjectRoleName*>(role1) ) {
			++CurrentDepth;
			expr.getC()->accept(*this);
			--CurrentDepth;
		} else if  (dynamic_cast<const  TDLObjectRoleInverse*>(role1) ) {
			expr.getC()->accept(*this);
		};
	}

	int getMaxDepth() {
		std::map<TConceptExpr*, int>::iterator im = DepthOfNominalOccurences.begin();
		if (im == DepthOfNominalOccurences.end() ) {
				return -1;
		} else {
			for (std::map<TConceptExpr*, int>::iterator it = DepthOfNominalOccurences.begin();
														it != DepthOfNominalOccurences.end(); ++it) {
				if ((*it). second > (*im).second) {
					im = it;
				}
			}
			return (*im).second;
		}
	}


	TConceptExpr* getNominalWithMaxDepth() {

		std::map<TConceptExpr*, int>::iterator im = DepthOfNominalOccurences.begin();

		if (im == DepthOfNominalOccurences.end() ) {
				return NULL;
		} else {
			for (std::map<const TConceptExpr*, int>::iterator it = DepthOfNominalOccurences.begin();
														it != DepthOfNominalOccurences.end(); ++it) {
				if ((*it). second >= (*im).second) {
					im = it;
				}
			}
			return (*im).first;
		}
	}

	void PrintDepthTable() {
		std::cout << "Total nominal occurrences: " << TotalNominalOccurences << "\n";
		TLISPExpressionPrinter pr(std::cout);
		for (std::map<const TConceptExpr *, int>::iterator it = DepthOfNominalOccurences.begin();
														it != DepthOfNominalOccurences.end(); ++it) {
			(*it).first -> accept(pr);
			std::cout << " has depth " << (*it).second << "\n";
		}
	}

}; // TDepthMeasurer

class TExpressionMarker: public DLExpressionVisitorEmpty
{
protected:	// members
	std::map<const TConceptExpr*, bool> GoodTerms;
	// A term is called good, if all of its subterms don't contain nominals different from x
	std::map<const TConceptExpr*, bool> SimpleTerms;
	std::vector<const TConceptExpr*> Path;
	TConceptExpr * Nominal;

public:		// interface
		/// init c'tor
	TExpressionMarker ( TConceptExpr * nominal) : Nominal(nominal) {}
		/// empty d'tor
	virtual ~TExpressionMarker ( void ) {}

public:
	// concept expressions

	virtual void visit ( const TDLConceptTop& expr ATTR_UNUSED ) {
		SimpleTerms.insert(std::pair<TConceptExpr*, bool>( &(expr), false));
		GoodTerms.insert(std::pair<TConceptExpr*, bool>( &(expr), true));
	}

	virtual void visit ( const TDLConceptName& expr )
	{
		SimpleTerms.insert(std::pair<TConceptExpr*, bool>( &(expr) , IsNominal(&expr) ));
		if ( (&expr) == Nominal) {
			GoodTerms.insert(std::pair<TConceptExpr*, bool>( &(expr), true));
			Path.push_back(&(expr));
		} else {
			GoodTerms.insert(std::pair<TConceptExpr*, bool>( &(expr), ! IsNominal(&expr) ));
		}
	}

	virtual void visit ( const TDLConceptAnd& expr ) {
		bool simple, good, onPath;
		simple = false, good = true, onPath = false;

		for (TDLNAryExpression<TConceptExpr>::iterator p = expr.begin(), p_end = expr.end(); p != p_end; ++p ) {
			(*p)->accept(*this);
			if (KnownToBeSimple(*p)) {
				simple = true;
		}
			if ( ! KnownToBeGood(*p)) {
				good = false;
			}
			if (  KnownToBeOnPath(*p)) {
				onPath = true;
			}
		}

		SimpleTerms.insert(std::pair<const TConceptExpr*, bool>( &(expr), simple));
		GoodTerms.insert(std::pair<const TConceptExpr*, bool>( &(expr), good));
		if (onPath && good && simple) {
			Path.push_back(& expr);
		}
	}


	virtual void visit ( const TDLConceptObjectExists& expr ) {
		const TDLObjectRoleExpression* role1 = expr.getOR();
		if ( dynamic_cast<const  TDLObjectRoleName*>(role1) ) {
			expr.getC()->accept(*this);
			SimpleTerms.insert(std::pair<TConceptExpr*, bool>( &(expr), false));
		} else if ( dynamic_cast<const  TDLObjectRoleInverse*>(role1) ) {
			expr.getC()->accept(*this);
			if (KnownToBeSimple(expr.getC())) {
				SimpleTerms.insert(std::pair<const TConceptExpr*, bool>( &(expr), true));
			} else {
				SimpleTerms.insert(std::pair<const TConceptExpr*, bool>( &(expr), false));
			}
		} else fpp_unreachable();
		GoodTerms.insert(std::pair<TConceptExpr*, bool>( &(expr), KnownToBeGood(expr.getC()) ));
		if (KnownToBeOnPath(expr.getC()) && KnownToBeGood(&expr) && KnownToBeSimple(& expr) ) {
			Path.push_back(&expr);
		}
	}

	bool KnownToBeSimple(const  TConceptExpr* expr) {
		return SimpleTerms[expr];
	}

	bool KnownToBeGood(const  TConceptExpr* expr) {
		return GoodTerms[expr];
	}
	bool KnownToBeOnPath(const  TConceptExpr* expr) {
		return (Path.size() >= 1 && Path[Path.size() - 1 ] == expr);
	}

	void PrintPath() {
		TLISPExpressionPrinter pr(std::cout);
		for ( size_t i = 0; i < Path.size(); ++i )
		{
			std::cout << "Expression on depth " << i << " :\n";
			Path[i] -> accept(pr);
			std::cout << "\n";
		}
	}

	TConceptExpr * getSubterm() {
		if (Path.size() >= 1) {
			return Path[Path.size() - 1];
		} else {
			return NULL;
		}
	}

}; // TExpressionMarker

class TReplacer: public DLExpressionVisitorEmpty
{
protected:	// members
	std::map<const TConceptExpr*, const TConceptExpr*> ReplaceResult;
	TConceptExpr * ExpressionToReplace;
	TConceptExpr * PropositionalVariable;

public:		// interface
		/// init c'tor
	TReplacer ( TConceptExpr* expression, const std::string& propositionalVariable )
		: ExpressionToReplace(expression)
		, PropositionalVariable(pEM->Concept(propositionalVariable))
		{}
		/// empty d'tor
	virtual ~TReplacer ( void ) {}

public:		// visitor interface
	// concept expressions

	virtual void visit ( const TDLConceptTop& expr ATTR_UNUSED ) {
		ReplaceResult.insert(std::pair<TConceptExpr*, TConceptExpr *>( &(expr), &(expr)));

	}
	virtual void visit ( const TDLConceptName& expr )
	{   if ( (&expr) == ExpressionToReplace) {
			ReplaceResult.insert(std::pair<TConceptExpr*, TConceptExpr *>( &(expr), PropositionalVariable));
		} else {
			ReplaceResult.insert(std::pair<TConceptExpr*, TConceptExpr *>( &(expr), &(expr) ));
		}
	}

	virtual void visit ( const TDLConceptAnd& expr ) {
		if ( (&expr) == ExpressionToReplace) {
			ReplaceResult.insert(std::pair<TConceptExpr*, TConceptExpr *>( &(expr), PropositionalVariable ));
		} else {
			TConceptExpr* s = NULL;
			for (TDLNAryExpression<TConceptExpr>::iterator p = expr.begin(), p_end = expr.end(); p != p_end; ++p ) {
				(*p)->accept(*this);
				if (p == expr.begin()) {
					s = ReplaceResult[*p];
				} else {
					s = And(s, ReplaceResult[*p]);
				}
			}
			ReplaceResult.insert(std::pair<const TConceptExpr*, const TConceptExpr*>( &(expr), s));
		}
	}


	virtual void visit ( const TDLConceptObjectExists& expr ) {
		if ( (&expr) == ExpressionToReplace) {
			ReplaceResult.insert(std::pair<TConceptExpr*, TConceptExpr *>( &(expr), PropositionalVariable));
		} else {
			const TDLObjectRoleExpression* role = expr.getOR();
			expr.getC() -> accept (* this);
			ReplaceResult.insert(std::pair<TConceptExpr*, TConceptExpr*>( &(expr), pEM->Exists(role, ReplaceResult[expr.getC()])));
		}

	}

	TConceptExpr*  getReplaceResult(TConceptExpr * c) { return ReplaceResult[c]; }
}; // TReplacer

class TEquationSolver {
	const TConceptExpr * LeftPart;
	const TConceptExpr * RightPart;
	TExpressionMarker * ExpressionMarker;
public:
	TEquationSolver(TConceptExpr * leftPart, std::string propositionalVariable, TExpressionMarker * expressionMarker)
	: LeftPart(leftPart)
	, RightPart(pEM -> Concept (propositionalVariable))
	, ExpressionMarker(expressionMarker)
	{}

	void Solve() {
		while(! IsNominal(LeftPart)) {
			if ( const TDLConceptObjectExists * leftDiamond = dynamic_cast<const TDLConceptObjectExists *>(LeftPart)) {
				const TDLObjectRoleInverse* invRole = dynamic_cast<const TDLObjectRoleInverse*>(leftDiamond -> getOR() ) ;
				const TDLObjectRoleExpression * role = invRole -> getOR();
				const TConceptExpr * newLeftPart = leftDiamond -> getC();
				const TConceptExpr * newRightPart = pEM -> Forall(role, RightPart);
				LeftPart = newLeftPart;
				RightPart = newRightPart;

			} else if (const TDLConceptAnd * leftAnd = dynamic_cast<const TDLConceptAnd *>(LeftPart)) {
				const TConceptExpr * arg1 = *(leftAnd -> begin());
				const TConceptExpr * arg2 = *(leftAnd -> begin() + 1);
				if ( ! ExpressionMarker -> KnownToBeSimple(arg1)) {
					const TConceptExpr * t;
					t = arg1;
					arg1 = arg2;
					arg2 = t;
				}
				const TConceptExpr * newLeftPart = arg1;
				const TConceptExpr * newRightPart;

				if (dynamic_cast<const TDLConceptTop * > (arg2) == NULL){
					newRightPart = pEM -> Or(pEM -> Not(arg2), RightPart);
				} else {
					newRightPart = RightPart;
				}
				LeftPart = newLeftPart;
				RightPart = newRightPart;
			}
		}
	}

	std::string getNominal() {
		const TDLConceptName * conceptName = dynamic_cast<const TDLConceptName * >(LeftPart);
		const std::string& longNominal = conceptName -> getName();
		int colon = longNominal.find(":");
		return std::string(longNominal, 0, colon);
	}

	TConceptExpr * getPhi() {
		return RightPart;
	}
}; // TEquationSolver

class TQueryToConceptsTransformer
{
public:		// types
	typedef std::multimap<std::string, TConceptExpr*> ResultType;

protected:	// members
		/// query to transform
	QRQuery* Query;
		/// transformation result
	ResultType Result;

public:		// interface
		/// init c'tor
	TQueryToConceptsTransformer ( QRQuery* query ) : Query(query) {}
		/// empty d'tor
	~TQueryToConceptsTransformer ( void ) {}

		/// main method to do the work
	void Run ( void )
	{
		transformQueryPhase1(Query);
		std::cout << "After Phase 1\n" << Query;
		TConceptExpr * term = transformQueryPhase2 (Query);
		TLISPExpressionPrinter pr(std::cout);
		char propositionalVariable[20];
		std::string lastNominal;
		for (int i = 1; true ; ++i   )
		{
			std::cout << "Expression:";
			term -> accept(pr);
			std::cout << "; i = " << i << "\n";

			std::cout << "Depth Measuring:";
			TDepthMeasurer depthMeasurer;
			term -> accept(depthMeasurer);
			std::cout << depthMeasurer.getMaxDepth() << std::endl;
			if (depthMeasurer.getMaxDepth() == -1) {
				break;
			}
			TConceptExpr * nominal = depthMeasurer.getNominalWithMaxDepth();
			std::cout << "Chosen nominal :";
			nominal -> accept (pr);
			std::cout << "\n";
			//depthMeasurer.PrintDepthTable();

			TExpressionMarker expressionMarker (nominal);
			term -> accept(expressionMarker);
			std::cout << "Simple ?" << expressionMarker.KnownToBeSimple(term) << "\n";
			expressionMarker.PrintPath();
			std::cout << "Going to replace subterm ";
			expressionMarker.getSubterm() -> accept(pr);
			std::cout << "\n";
			std::cout << "Initializing Replacer...\n";
			sprintf(propositionalVariable, "P%d", i);
			TReplacer replacer(expressionMarker.getSubterm(), propositionalVariable );
			std::cout << "Running Replacer...\n";
			term -> accept(replacer);
			std::cout << "Replace Result :\n";
			replacer.getReplaceResult(term) -> accept(pr);
			std::cout << "\n";
			std::cout << "Initializing Solver...\n";
			TEquationSolver equationSolver(expressionMarker.getSubterm(), propositionalVariable , & expressionMarker);
			std::cout << "Running Solver...\n";
			equationSolver. Solve();
			std::cout << "Phi : ";
			equationSolver. getPhi() -> accept(pr);
			Result.insert(std::pair<std::string, TConceptExpr *>(equationSolver . getNominal(), equationSolver.getPhi()));
			std::cout << "\nNominal: " << equationSolver . getNominal() << "\n";
			lastNominal = equationSolver.getNominal();
			term = replacer.getReplaceResult(term);
		}
		Result.insert(std::pair<std::string, TConceptExpr *>(lastNominal, pEM -> Not(pEM->Concept(propositionalVariable))));
	}

		/// get the result
	const ResultType& getResult( void ) const { return Result; }
		/// print the result
	void printResult ( void ) const
	{
		TLISPExpressionPrinter pr(std::cerr);
		for ( ResultType::const_iterator it = Result.begin(), end = Result.end(); it != end; ++it)
		{
			std::cerr << it->first << ": ";
			(it->second)->accept(pr);
			std::cerr << "\n";
		}
	}
}; // TQueryToConceptsTransformer

void doQuery ( QRQuery* query, ReasoningKernel* kernel )
{
	std::cout << "Next query: " << query;

	QueryConnectednessChecker cc1(query);
	std::cout << "Connected? " << cc1.isConnected() << "\n";

	TQueryToConceptsTransformer transformer(query);
	transformer.Run();
	transformer.printResult();
	kernel->evaluateQuery(transformer.getResult());
}
