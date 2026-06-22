#pragma once

#include <Poseidon/Foundation/Framework/DebugLog.hpp>



namespace Poseidon
{
class TreeItem
{
	public:
	int _key;
	TreeItem *_bigger,*_smaller;
};

template <class Type,class Base=TreeItem,class Index=int>
class SortTree
{
	private:
	TreeItem *_root;
	
	public:
	SortTree();
	~SortTree();

	void Insert( const Type &item, Index key );
	void Delete( Index key );
	Type *Find( Index key ) const;
	Type *FindNearestBiggerOrEqual( Index key ) const;
	Type *FindNearestBigger( Index key ) const;
	Type *FindNearestLower( Index key ) const;
	Type *FindNearestLowerOrEqual( Index key ) const;
};

template <class Type,class Base,class Index>
SortTree<Type,Base,Index>::SortTree()
:_root(nullptr)
{
}

template <class Type,class Base,class Index>
SortTree<Type,Base,Index>::~SortTree()
{
	while( _root ) Delete(_root->_key);
}

template <class Type,class Base,class Index>
void SortTree<Type,Base,Index>::Insert( const Type &item, Index key )
{
	TreeItem *node;
	TreeItem **back;
	for( node=_root,back=&_root; node; )
	{
		if( node->_key>key ) node=node->_bigger;
		else if( node->_key<key ) node=node->_smaller;
		else
		{
			Fail("SortTree::Insert - key already present"); // equal - take any
		}
	}
}

template <class Type,class Base,class Index>
void SortTree<Type,Base,Index>::Delete( Index key )
{
	TreeItem *item;
	TreeItem **back;
	for( item=_root,back=&_root; item; )
	{
		if( item->_key>key ) back=&item->_bigger,item=item->_bigger;
		else if( item->_key<key ) back=&item->_smaller,item=item->_smaller;
		else
		{
			// item found - delete it
			// to do: sometimes use bigger, sometimes smaller (counter++&1)
			TreeItem *b=item->_bigger;
			TreeItem *s=item->_smaller;
			// find place for b->_smaller
			// find place for biggest of small
			TreeItem *sbEnd;
			for( sbEnd=s; sbEnd->_bigger; sbEnd=sbEnd->_bigger ){}
			// reorder tree
			(*back)=b; // remove item
			sbEnd->_bigger=b->_smaller; // move b->_smaller
			b->_smaller=s; // move s
		}
	}
	Fail("SortTree::Delete - key not found");
}

template <class Type,class Base,class Index>
Type *SortTree<Type,Base,Index>::Find( Index key ) const
{
	TreeItem *item;
	for( item=_root; item; )
	{
		if( item->_key>key ) item=item->_bigger;
		else if( item->_key<key ) item=item->_smaller;
		else return static_cast<Type *>(item); // equal
	}
	return nullptr;
}
} // namespace Poseidon
