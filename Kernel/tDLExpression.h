/* This file is part of the FaCT++ DL reasoner
Copyright (C) 2010 by Dmitry Tsarkov

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

#ifndef TDLEXPRESSION_H
#define TDLEXPRESSION_H

#include <vector>
#include <string>

#include "eFaCTPlusPlus.h"

// forward declaration for all expression classes: necessary for the visitor pattern
class TDLExpression;

class  TDLConceptExpression;
class   TDLConceptTop;
class   TDLConceptBottom;
class   TDLConceptName;

class  TDLIndividualExpression;
class   TDLIndividualName;

class  TDLRoleExpression;
class   TDLObjectRoleComplexExpression;
class    TDLObjectRoleExpression;
class     TDLObjectRoleTop;
class     TDLObjectRoleBottom;
class     TDLObjectRoleName;
class     TDLObjectRoleInverse;
class    TDLObjectRoleChain;
class    TDLObjectRoleProjectionFrom;
class    TDLObjectRoleProjectionInto;

class   TDLDataRoleExpression;
class    TDLDataRoleTop;
class    TDLDataRoleBottom;
class    TDLDataRoleName;

class  TDLDataExpression;
class   TDLDataTop;
class   TDLDataBottom;
class   TDLDataTypeName;

/// general visitor for DL expressions
class DLExpressionVisitor
{
public:		// visitor interface
	// concept expressions
	virtual void visit ( TDLConceptTop& expr ) = 0;
	virtual void visit ( TDLConceptBottom& expr ) = 0;
	virtual void visit ( TDLConceptName& expr ) = 0;

	// individual expressions
	virtual void visit ( TDLIndividualName& expr ) = 0;

	// object role expressions
	virtual void visit ( TDLObjectRoleTop& expr ) = 0;
	virtual void visit ( TDLObjectRoleBottom& expr ) = 0;
	virtual void visit ( TDLObjectRoleName& expr ) = 0;
	virtual void visit ( TDLObjectRoleInverse& expr ) = 0;
	virtual void visit ( TDLObjectRoleChain& expr ) = 0;
	virtual void visit ( TDLObjectRoleProjectionFrom& expr ) = 0;
	virtual void visit ( TDLObjectRoleProjectionInto& expr ) = 0;

	// data role expressions
	virtual void visit ( TDLDataRoleTop& expr ) = 0;
	virtual void visit ( TDLDataRoleBottom& expr ) = 0;
	virtual void visit ( TDLDataRoleName& expr ) = 0;

	// data expressions
	virtual void visit ( TDLDataTop& expr ) = 0;
	virtual void visit ( TDLDataBottom& expr ) = 0;
	virtual void visit ( TDLDataTypeName& expr ) = 0;

	// other methods
	virtual ~DLExpressionVisitor ( void ) {}
}; // DLExpressionVisitor


/// base class for the DL expression, which include concept-, (data)role-, individual-, and data ones
class TDLExpression
{
public:		// interface
		/// empty c'tor
	TDLExpression ( void ) {}
		/// empty d'tor: note that no deep delete is necessary as all the elements are RO
	virtual ~TDLExpression ( void ) {}

		/// accept method for the visitor pattern
	virtual void accept ( DLExpressionVisitor& visitor ) = 0;
}; // TDLExpression


//------------------------------------------------------------------
//	helper classes
//------------------------------------------------------------------


//------------------------------------------------------------------
///	named entity
//------------------------------------------------------------------
class TNamedEntity
{
protected:	// members
		/// name of the entity
	std::string Name;

public:		// interface
		/// c'tor: initialise name
	TNamedEntity ( const std::string& name ) : Name(name) {}
		/// empty d'tor
	virtual ~TNamedEntity ( void ) {}

		/// get access to the name
	const char* getName ( void ) const { return Name.c_str(); }
}; // TNamedEntity

//------------------------------------------------------------------
///	concept argument
//------------------------------------------------------------------
class TConceptArg
{
protected:	// members
		/// concept argument
	TDLConceptExpression* C;

public:		// interface
		/// init c'tor
	TConceptArg ( TDLConceptExpression* c ) : C(c) {}
		/// empty d'tor
	virtual ~TConceptArg ( void ) {}

		/// get access to the argument
	TDLConceptExpression* getC ( void ) const { return C; }
}; // TConceptArg

//------------------------------------------------------------------
///	object role argument
//------------------------------------------------------------------
class TObjectRoleArg
{
protected:	// members
		/// object role argument
	TDLObjectRoleExpression* OR;

public:		// interface
		/// init c'tor
	TObjectRoleArg ( TDLObjectRoleExpression* oR ) : OR(oR) {}
		/// empty d'tor
	virtual ~TObjectRoleArg ( void ) {}

		/// get access to the argument
	TDLObjectRoleExpression* getOR ( void ) const { return OR; }
}; // TObjectRoleArg

//------------------------------------------------------------------
///	data role argument
//------------------------------------------------------------------
class TDataRoleArg
{
protected:	// members
		/// data role argument
	TDLDataRoleExpression* DR;

public:		// interface
		/// init c'tor
	TDataRoleArg ( TDLDataRoleExpression* dR ) : DR(dR) {}
		/// empty d'tor
	virtual ~TDataRoleArg ( void ) {}

		/// get access to the argument
	TDLDataRoleExpression* getDR ( void ) const { return DR; }
}; // TDataRoleArg

//------------------------------------------------------------------
///	data expression argument (templated with the exact type)
//------------------------------------------------------------------
template<class TExpression>
class TDataExpressionArg
{
protected:	// members
		/// data expression argument
	TExpression* Expr;

public:		// interface
		/// init c'tor
	TDataExpressionArg ( TExpression* expr ) : Expr(expr) {}
		/// empty d'tor
	virtual ~TDataExpressionArg ( void ) {}

		/// get access to the argument
	TExpression* getExpr ( void ) const { return Expr; }
}; // TDataExpressionArg

//------------------------------------------------------------------
///	general n-argument expression
//------------------------------------------------------------------
template<class Argument>
class TDLNAryExpression
{
public:		// types
		/// base type
	typedef std::vector<Argument*> ArgumentArray;
		/// RW iterator over base type
	typedef typename ArgumentArray::const_iterator iterator;
		/// input array type
	typedef std::vector<TDLExpression*> ExpressionArray;
		/// RW input iterator
	typedef ExpressionArray::const_iterator i_iterator;

protected:	// members
		/// set of equivalent concept descriptions
	ArgumentArray Base;
		/// name for excepion depending on class name and direction
	std::string EString;

protected:	// methods
		/// transform general expression into the argument one
	Argument* transform ( TDLExpression* arg )
	{
		Argument* p = dynamic_cast<Argument*>(arg);
		if ( p == NULL )
			throw EFaCTPlusPlus(EString.c_str());
		return p;
	}

public:		// interface
		/// c'tor: build an error string
	TDLNAryExpression ( const char* typeName, const char* className )
	{
		EString = "Expected ";
		EString += typeName;
		EString += " argument in the '";
		EString += className;
		EString += "' expression";
	}
		/// empty d'tor
	virtual ~TDLNAryExpression ( void ) {}

	// add elements to the array

		/// add a single element to the array
	void add ( TDLExpression* p ) { Base.push_back(transform(p)); }
		/// add a range to the array
	void add ( i_iterator b, i_iterator e )
	{
		for ( ; b != e; ++b )
			add(*b);
	}
		/// add a vector
	void add ( const ExpressionArray& v ) { add ( v.begin(), v.end() ); }

	// access to members

		/// RW begin iterator for array
	iterator begin ( void ) { return Base.begin(); }
		/// RW end iterator for array
	iterator end ( void ) { return Base.end(); }
}; // TDLNAryExpression


//------------------------------------------------------------------
//	concept expressions
//------------------------------------------------------------------


//------------------------------------------------------------------
///	general concept expression
//------------------------------------------------------------------
class TDLConceptExpression: public TDLExpression
{
public:		// interface
		/// empty c'tor
	TDLConceptExpression ( void ) : TDLExpression() {}
		/// empty d'tor
	virtual ~TDLConceptExpression ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) = 0;
}; // TDLConceptExpression

//------------------------------------------------------------------
///	concept TOP expression
//------------------------------------------------------------------
class TDLConceptTop: public TDLConceptExpression
{
public:		// interface
		/// empty c'tor
	TDLConceptTop ( void ) : TDLConceptExpression() {}
		/// empty d'tor
	virtual ~TDLConceptTop ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLConceptTop

//------------------------------------------------------------------
///	concept BOTTOM expression
//------------------------------------------------------------------
class TDLConceptBottom: public TDLConceptExpression
{
public:		// interface
		/// empty c'tor
	TDLConceptBottom ( void ) : TDLConceptExpression() {}
		/// empty d'tor
	virtual ~TDLConceptBottom ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLConceptBottom

//------------------------------------------------------------------
///	named concept expression
//------------------------------------------------------------------
class TDLConceptName: public TDLConceptExpression, public TNamedEntity
{
public:		// interface
		/// init c'tor
	TDLConceptName ( const std::string& name ) : TDLConceptExpression(), TNamedEntity(name) {}
		/// empty d'tor
	virtual ~TDLConceptName ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLConceptName


//------------------------------------------------------------------
//	individual expressions
//------------------------------------------------------------------


//------------------------------------------------------------------
///	general individual expression
//------------------------------------------------------------------
class TDLIndividualExpression: public TDLExpression
{
public:		// interface
		/// empty c'tor
	TDLIndividualExpression ( void ) : TDLExpression() {}
		/// empty d'tor
	virtual ~TDLIndividualExpression ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) = 0;
}; // TDLIndividualExpression

//------------------------------------------------------------------
///	named individual expression
//------------------------------------------------------------------
class TDLIndividualName: public TDLIndividualExpression, public TNamedEntity
{
public:		// interface
		/// init c'tor
	TDLIndividualName ( const std::string& name ) : TDLIndividualExpression(), TNamedEntity(name) {}
		/// empty d'tor
	virtual ~TDLIndividualName ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLIndividualName


//------------------------------------------------------------------
//	role expressions
//------------------------------------------------------------------


//------------------------------------------------------------------
///	general role expression
//------------------------------------------------------------------
class TDLRoleExpression: public TDLExpression
{
public:		// interface
		/// empty c'tor
	TDLRoleExpression ( void ) : TDLExpression() {}
		/// empty d'tor
	virtual ~TDLRoleExpression ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) = 0;
}; // TDLRoleExpression


//------------------------------------------------------------------
//	object role expressions
//------------------------------------------------------------------


//------------------------------------------------------------------
///	complex object role expression (general expression, role chain or projection)
//------------------------------------------------------------------
class TDLObjectRoleComplexExpression: public TDLRoleExpression
{
public:		// interface
		/// empty c'tor
	TDLObjectRoleComplexExpression ( void ) : TDLRoleExpression() {}
		/// empty d'tor
	virtual ~TDLObjectRoleComplexExpression ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) = 0;
}; // TDLObjectRoleComplexExpression

//------------------------------------------------------------------
///	general object role expression
//------------------------------------------------------------------
class TDLObjectRoleExpression: public TDLObjectRoleComplexExpression
{
public:		// interface
		/// empty c'tor
	TDLObjectRoleExpression ( void ) : TDLObjectRoleComplexExpression() {}
		/// empty d'tor
	virtual ~TDLObjectRoleExpression ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) = 0;
}; // TDLObjectRoleExpression

//------------------------------------------------------------------
///	object role TOP expression
//------------------------------------------------------------------
class TDLObjectRoleTop: public TDLObjectRoleExpression
{
public:		// interface
		/// empty c'tor
	TDLObjectRoleTop ( void ) : TDLObjectRoleExpression() {}
		/// empty d'tor
	virtual ~TDLObjectRoleTop ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLObjectRoleTop

//------------------------------------------------------------------
///	object role BOTTOM expression
//------------------------------------------------------------------
class TDLObjectRoleBottom: public TDLObjectRoleExpression
{
public:		// interface
		/// empty c'tor
	TDLObjectRoleBottom ( void ) : TDLObjectRoleExpression() {}
		/// empty d'tor
	virtual ~TDLObjectRoleBottom ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLObjectRoleBottom

//------------------------------------------------------------------
///	named object role expression
//------------------------------------------------------------------
class TDLObjectRoleName: public TDLObjectRoleExpression, public TNamedEntity
{
public:		// interface
		/// init c'tor
	TDLObjectRoleName ( const std::string& name ) : TDLObjectRoleExpression(), TNamedEntity(name) {}
		/// empty d'tor
	virtual ~TDLObjectRoleName ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLObjectRoleName

//------------------------------------------------------------------
///	inverse object role expression
//------------------------------------------------------------------
class TDLObjectRoleInverse: public TDLObjectRoleExpression, public TObjectRoleArg
{
public:		// interface
		/// init c'tor
	TDLObjectRoleInverse ( TDLObjectRoleExpression* R )
		: TDLObjectRoleExpression()
		, TObjectRoleArg(R)
		{}
		/// empty d'tor
	virtual ~TDLObjectRoleInverse ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLObjectRoleInverse

//------------------------------------------------------------------
/// object role chain expression
//------------------------------------------------------------------
class TDLObjectRoleChain: public TDLObjectRoleComplexExpression, public TDLNAryExpression<TDLObjectRoleExpression>
{
public:		// interface
		/// init c'tor: create role chain from given array
	TDLObjectRoleChain ( const ExpressionArray& v )
		: TDLObjectRoleComplexExpression()
		, TDLNAryExpression<TDLObjectRoleExpression>("object role expression","role chain")
	{
		add(v);
	}
		/// empty d'tor
	virtual ~TDLObjectRoleChain ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLObjectRoleChain

//------------------------------------------------------------------
///	object role projection from expression
//------------------------------------------------------------------
class TDLObjectRoleProjectionFrom
	: public TDLObjectRoleComplexExpression
	, public TObjectRoleArg
	, public TConceptArg
{
public:		// interface
		/// init c'tor
	TDLObjectRoleProjectionFrom ( TDLObjectRoleExpression* R, TDLConceptExpression* C )
		: TDLObjectRoleComplexExpression()
		, TObjectRoleArg(R)
		, TConceptArg(C)
		{}
		/// empty d'tor
	virtual ~TDLObjectRoleProjectionFrom ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLObjectRoleProjectionFrom

//------------------------------------------------------------------
///	object role projection from expression
//------------------------------------------------------------------
class TDLObjectRoleProjectionInto
	: public TDLObjectRoleComplexExpression
	, public TObjectRoleArg
	, public TConceptArg
{
public:		// interface
		/// init c'tor
	TDLObjectRoleProjectionInto ( TDLObjectRoleExpression* R, TDLConceptExpression* C )
		: TDLObjectRoleComplexExpression()
		, TObjectRoleArg(R)
		, TConceptArg(C)
		{}
		/// empty d'tor
	virtual ~TDLObjectRoleProjectionInto ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLObjectRoleProjectionInto


//------------------------------------------------------------------
//	data role expressions
//------------------------------------------------------------------


//------------------------------------------------------------------
///	general data role expression
//------------------------------------------------------------------
class TDLDataRoleExpression: public TDLRoleExpression
{
public:		// interface
		/// empty c'tor
	TDLDataRoleExpression ( void ) : TDLRoleExpression() {}
		/// empty d'tor
	virtual ~TDLDataRoleExpression ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) = 0;
}; // TDLDataRoleExpression

//------------------------------------------------------------------
///	data role TOP expression
//------------------------------------------------------------------
class TDLDataRoleTop: public TDLDataRoleExpression
{
public:		// interface
		/// empty c'tor
	TDLDataRoleTop ( void ) : TDLDataRoleExpression() {}
		/// empty d'tor
	virtual ~TDLDataRoleTop ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLDataRoleTop

//------------------------------------------------------------------
///	data role BOTTOM expression
//------------------------------------------------------------------
class TDLDataRoleBottom: public TDLDataRoleExpression
{
public:		// interface
		/// empty c'tor
	TDLDataRoleBottom ( void ) : TDLDataRoleExpression() {}
		/// empty d'tor
	virtual ~TDLDataRoleBottom ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLDataRoleBottom

//------------------------------------------------------------------
///	named data role expression
//------------------------------------------------------------------
class TDLDataRoleName: public TDLDataRoleExpression, public TNamedEntity
{
public:		// interface
		/// init c'tor
	TDLDataRoleName ( const std::string& name ) : TDLDataRoleExpression(), TNamedEntity(name) {}
		/// empty d'tor
	virtual ~TDLDataRoleName ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLDataRoleName


//------------------------------------------------------------------
//	data expressions
//------------------------------------------------------------------


//------------------------------------------------------------------
///	general data expression
//------------------------------------------------------------------
class TDLDataExpression: public TDLExpression
{
public:		// interface
		/// empty c'tor
	TDLDataExpression ( void ) : TDLExpression() {}
		/// empty d'tor
	virtual ~TDLDataExpression ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) = 0;
}; // TDLDataExpression

//------------------------------------------------------------------
///	data TOP expression
//------------------------------------------------------------------
class TDLDataTop: public TDLDataExpression
{
public:		// interface
		/// empty c'tor
	TDLDataTop ( void ) : TDLDataExpression() {}
		/// empty d'tor
	virtual ~TDLDataTop ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLDataTop

//------------------------------------------------------------------
///	data BOTTOM expression
//------------------------------------------------------------------
class TDLDataBottom: public TDLDataExpression
{
public:		// interface
		/// empty c'tor
	TDLDataBottom ( void ) : TDLDataExpression() {}
		/// empty d'tor
	virtual ~TDLDataBottom ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLDataBottom

//------------------------------------------------------------------
///	named data type expression
//------------------------------------------------------------------
class TDLDataTypeName: public TDLDataExpression, public TNamedEntity
{
public:		// interface
		/// init c'tor
	TDLDataTypeName ( const std::string& name ) : TDLDataExpression(), TNamedEntity(name) {}
		/// empty d'tor
	virtual ~TDLDataTypeName ( void ) {}

		/// accept method for the visitor pattern
	void accept ( DLExpressionVisitor& visitor ) { visitor.visit(*this); }
}; // TDLDataTypeName


#endif