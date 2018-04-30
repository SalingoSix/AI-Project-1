#include "cNode.h"

cNode::cNode()
{

}

cNode::cNode(int node)
{
	ID = node;
}

cNode::~cNode()
{

}

void cNode::addConnection(cNode* newNode)
{
	this->connectedNodes.push_back(newNode);
}