/* This file is part of the FaCT++ DL reasoner
Copyright (C) 2003-2008 by Dmitry Tsarkov

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

#ifndef _ROLEMASTER_H
#define _ROLEMASTER_H

#include "globaldef.h"
#include "tNameSet.h"
#include "eFPPCantRegName.h"
#include "dltree.h"
#include "dlCompletionTree.h"
#include "tRole.h"
#include "Taxonomy.h"

class RoleMaster
{
public:		// types
	typedef TRole::roleSet roleSet;
	typedef TRole::RoleBitMap RoleBitMap;

		/// RW access to roles
	typedef roleSet::iterator iterator;
		/// RO access to roles
	typedef roleSet::const_iterator const_iterator;

protected:	// members
		/// number of the last registered role
	int newRoleId;

		/// all registered roles
	roleSet Roles;
		/// internal empty role (bottom in the taxonomy)
	TRole emptyRole;
		/// internal universal role (top in the taxonomy)
	TRole universalRole;

		/// roles nameset
	TNameSet<TRole> roleNS;
		/// Taxonomy of roles
	Taxonomy* pTax;

		/// two halves of disjoint roles axioms
	roleSet DJRolesA, DJRolesB;

		/// flag if it is possible to introduce new names
	bool useUndefinedNames;

private:	// methods
		/// no copy c'tor
	RoleMaster ( const RoleMaster& );
		/// no assignment
	RoleMaster& operator = ( const RoleMaster& );
		/// constant defining first user role in the RBox
	static unsigned int firstRoleIndex ( void ) { return 2; }

protected:	// methods
		/// register TRole and it's inverse in RoleBox
	void registerRole ( TRole* r, bool isDataRole )
	{
		assert ( r != NULL && r->Inverse == NULL );	// sanity check
		assert ( r->getId() == 0 );	// only call it for the new roles

		if ( isDataRole )
			r->setDataRole();

		Roles.push_back (r);
		r->setId (newRoleId);

		// create new role which would be inverse of R
		std::string iname ("-");
		iname += r->getName();
		TRole* ri = new TRole(iname);

		// set up inverse
		r->setInverse(ri);
		ri->setInverse(r);

		Roles.push_back (ri);
		ri->setId (-newRoleId);
		++newRoleId;
	}
		/// get number of roles
	size_t size ( void ) const { return Roles.size()/2-1; }

public:		// interface
		/// the only c'tor
	RoleMaster ( void )
		: newRoleId(1)
		, emptyRole("emptyRole")
		, universalRole("universalRole")
		, roleNS()
		, useUndefinedNames(true)
	{
		// no zero-named roles allowed
		Roles.push_back(NULL);
		Roles.push_back(NULL);
		// setup empty role
		emptyRole.setId(0);
		emptyRole.setInverse(&emptyRole);
		// setup universal role
		universalRole.setId(0);
		universalRole.setInverse(&universalRole);

		// create roles taxonomy
		pTax = new Taxonomy ( &universalRole, &emptyRole );
	}
		/// d'tor (delete taxonomy)
	~RoleMaster ( void ) { delete pTax; }

		/// @return true if P is a role that is registered in the RM
	bool isRegisteredRole ( const TNamedEntry* p ) const
	{
		const TRole* R = reinterpret_cast<const TRole*>(p);
		if ( R == NULL )
			return false;
		unsigned int ind = R->getIndex();
		return ( ind >= firstRoleIndex() &&
				 ind < Roles.size() &&
				 Roles[ind] == p );
	}

		/// create role entry with given name
	TNamedEntry* ensureRoleName ( const std::string& name, bool isDataRole ) throw(EFPPCantRegName)
	{
		TRole* p = roleNS.insert(name);
		// check what happens
		if ( p == NULL )			// role registration attempt failed
			throw EFPPCantRegName ( name, isDataRole ? "data role" : "role" );

		if ( isRegisteredRole(p) )	// registered role
			return p;
		if ( p->getId() != 0 ||		// not registered but has non-null ID
			 !useUndefinedNames )	// new names are disallowed
			throw EFPPCantRegName ( name, isDataRole ? "data role" : "role" );

		registerRole(p,isDataRole);
		return p;
	}

		/// add parent for the input role
	bool addRoleParent ( TRole* role, TRole* parent ) const
	{
		if ( role->isDataRole() != parent->isDataRole() )
			return true;

		role->addParent(parent);
		role->Inverse->addParent(parent->Inverse);
		return false;
	}

		/// add parent for the input role or role composition
	bool addRoleParent ( DLTree* role, TRole* parent );
		/// add synonym to existing role
	bool addRoleSynonym ( TRole* role, TRole* syn )
	{
		if ( role == syn )	// nothing to do
			return false;

		return addRoleParent ( role, syn ) || addRoleParent ( syn, role );
	}

		/// register a pair of disjoint roles
	void addDisjointRoles ( TRole* R, TRole* S )
	{
		DJRolesA.push_back(R);
		DJRolesB.push_back(S);
	}

		/// create taxonomy of roles (using the Parent data)
	void initAncDesc ( void );

		/// change the undefined names usage policy
	void setUndefinedNames ( bool val ) { useUndefinedNames = val; }

	// access to roles

		/// RW pointer to the first user-defined role
	iterator begin ( void ) { return Roles.begin()+firstRoleIndex(); }
		/// RW pointer after the last user-defined role
	iterator end ( void ) { return Roles.end(); }
		/// RO pointer to the first user-defined role
	const_iterator begin ( void ) const { return Roles.begin()+firstRoleIndex(); }
		/// RO pointer after the last user-defined role
	const_iterator end ( void ) const { return Roles.end(); }

		/// get access to the taxonomy
	Taxonomy* getTaxonomy ( void ) const { return pTax; }

		/// @return true iff there is a reflexive role
	bool hasReflexiveRoles ( void ) const;
		/// put all reflexive roles to a RR array
	void fillReflexiveRoles ( roleSet& RR ) const;

		/// get new bit-map big enough to keep all the roles from the RM
	RoleBitMap newBitMap ( void ) const { return RoleBitMap(Roles.size(),false); }
		/// @return a bitmap which corresponds role names that appears within [BEGIN,END) interval of the TRelated
	template<class Iterator>
	RoleBitMap buildRelatedRoles ( Iterator begin, Iterator end ) const;
		/// fills Rs with the role names that are in RBM and corresponds to DATA and NEEDI
	void getRelatedRoles ( const RoleBitMap& rbm, std::vector<TNamedEntry*>& Rs, bool data, bool needI ) const
	{
		Rs.clear();
		for ( unsigned int i = firstRoleIndex(); i < Roles.size(); ++i )
		{
			TRole* R = Roles[i];
			if ( R->isDataRole() == data && ( R->getId() > 0 || needI ) && rbm[i] )
				Rs.push_back(Roles[i]);
		}
	}

	// output interface

	void Print ( std::ostream& o ) const;

	// save/load interface; implementation is in SaveLoad.cpp

		/// save entry
	void Save ( std::ostream& o ) const;
		/// load entry
	void Load ( std::istream& i );
}; // RoleMaster

inline bool
RoleMaster :: hasReflexiveRoles ( void ) const
{
	for  ( const_iterator p = begin(); p != end(); ++p )
		if ( (*p)->isReflexive() )
			return true;

	return false;
}

inline void
RoleMaster :: fillReflexiveRoles ( roleSet& RR ) const
{
	RR.clear();
	for  ( const_iterator p = begin(); p != end(); ++p )
		if ( !(*p)->isSynonym() && (*p)->isReflexive() )
			RR.push_back(*p);
}

template<class Iterator> RoleMaster::RoleBitMap
RoleMaster :: buildRelatedRoles ( Iterator begin, Iterator end ) const
{
	size_t i;
	RoleBitMap local = newBitMap();

	// fill roles with just named roles from the array
	for ( ; begin < end; ++begin )
		local[(*begin)->R->getIndex()] = true;
	// add all the ancestors of the role to the bitmap
	for ( i = firstRoleIndex(); i < Roles.size(); ++i )
		if ( local[i] )
			Roles[i]->addAncestorsToBitMap(local);
	// add synonyms to the map
	for ( i = firstRoleIndex(); i < Roles.size(); ++i )
		if ( Roles[i]->isSynonym() && local[resolveSynonym(Roles[i])->getIndex()] )
			local[i] = true;
	return local;
}

#endif