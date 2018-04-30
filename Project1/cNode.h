#ifndef _HG_cNode_
#define _HG_cNode_

#include <vector>

class cNode
{
public:
	cNode();
	cNode(int);
	~cNode();

	int ID;
	std::vector<cNode*> connectedNodes;

	void addConnection(cNode*);

};

#endif