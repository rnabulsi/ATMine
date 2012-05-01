#ifndef TREE_H
#define TREE_H

#include <QList>
#include <QDataStream>

template<class NodeType, class EdgeType>
class Node {
private:
	QList<Node<NodeType, EdgeType> * > children;
	QList<EdgeType > edges;
	NodeType value;
public:
	Node(){}
	Node(const NodeType & val): value(val) {}
	const NodeType & getValue() const { return value;}
	Node<NodeType, EdgeType> * getChild(int i) const { return children[i];}
	const EdgeType & getEdge(int i) const {return edges[i];}
	int size() const { return children.size();}
	void addNode(const EdgeType & edge,Node<NodeType, EdgeType> * node) { children.append(node); edges.append(edge);}
	bool isLeaf() const {return size()==0;}
};


template <class NodeType, class EdgeType>
class Tree {
private:
	Node<NodeType, EdgeType> * root;
private:
	void deleteHelper(Node<NodeType, EdgeType> * subroot) {
		for (int i=0;i<subroot->size();i++) {
			delete subroot->getChild(i);
		}
		delete subroot;
	}

public:
	Tree() { root=NULL;}
	Tree(Node<NodeType, EdgeType> * root) {this->root=root;}
	void setRoot(Node<NodeType, EdgeType> * root) {this->root=root;}
	Node<NodeType, EdgeType> * getRoot() const {return root;}
	~Tree() {deleteHelper(root);}
};

template <class NodeType, class EdgeType>
inline QDataStream &operator<<(QDataStream &out, const Node<NodeType, EdgeType> &t) {
	out<<t.getValue();
	int size=t.size();
	out<<size;
	for (int i=0;i<size;i++) {
		out<<t.getEdge(i);
		out<<*(t.getChild(i));
	}
	return out;
}

template <class NodeType, class EdgeType>
inline QDataStream &operator>>(QDataStream &in, Node<NodeType, EdgeType> &t) {
	in>>t.value;
	int size;
	in >>size;
	for (int i=0;i<size;i++) {
		EdgeType e;
		in >>e;
		Node<NodeType, EdgeType> * child=new Node<NodeType, EdgeType>();
		in>>*(child);
		t.addNode(e,child);
	}
	return in;
}

template <class NodeType, class EdgeType>
inline QDataStream &operator<<(QDataStream &out, const Tree<NodeType, EdgeType> &t) {
	out<<*(t.getRoot());
	return out;
}

template <class NodeType, class EdgeType>
inline QDataStream &operator>>(QDataStream &in, Tree<NodeType, EdgeType> &t) {
	Node<NodeType, EdgeType> * root=new Node<NodeType, EdgeType>();
	in>>*root;
	t.setRoot(root);
	return in;
}


#endif // TREE_H
