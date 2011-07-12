#include <QQueue>
#include "graph.h"

void ColorIndices::unUse(unsigned int bit)//unuse and clear color bit for all nodes in graph
{
	assert (bit<maxBits());
	usedBits &= (~(1 << bit));

	int max=graph->all_nodes.size();
	for (int i=0;i<max;i++) {
		graph->all_nodes[i]->resetVisited(bit);
		assert(!graph->all_nodes[i]->isVisited(bit));
		ChainNodeIterator itr=graph->all_nodes[i]->begin();
		for (;!itr.isFinished();++itr) {
			ChainNarratorNode & c=*itr;
			c.resetVisited(bit);
			assert(!c.isVisited(bit));
		}
	}

	if (nextUnused>bit)
		nextUnused=bit;
}

GraphVisitorController::GraphVisitorController(NodeVisitor * visitor,NarratorGraph * graph,unsigned int visitIndex,unsigned int finishIndex,bool keep_track_of_edges, bool merged_edges_as_one) //assumes keep_track_of_nodes by default
{
	construct(visitor,graph,keep_track_of_edges,true,merged_edges_as_one);
	this->visitIndex=visitIndex; //assumed already cleared
	graph->colorGuard.use(visitIndex);
	this->finishIndex=finishIndex; //assumed already cleared
	graph->colorGuard.use(finishIndex);
}
GraphVisitorController::GraphVisitorController(NodeVisitor * visitor,NarratorGraph * graph,bool keep_track_of_edges,bool keep_track_of_nodes, bool merged_edges_as_one)
{
	construct(visitor,graph,keep_track_of_edges,keep_track_of_nodes,merged_edges_as_one);
	if (keep_track_of_nodes)
	{
		visitIndex=graph->colorGuard.getNextUnused();
		graph->colorGuard.use(visitIndex);
		finishIndex=graph->colorGuard.getNextUnused();
		graph->colorGuard.use(finishIndex);
	}
}
void GraphVisitorController::initialize()
{
	init();
	//graph->colorGuard.use(visitIndex);
	visitor->initialize();
}
void GraphVisitorController::finish()
{
	graph->colorGuard.unUse(visitIndex);
	graph->colorGuard.unUse(finishIndex);
	visitor->finish();
}

void LoopBreakingVisitor::reMergeNodes(NarratorNodeIfc * n)
{
	if (n==NULL)
		return;
	if (!n->isGraphNode())
		return; //unable to resolve
	GraphNarratorNode * g=(GraphNarratorNode *)n;
#ifdef DISPLAY_NODES_BEING_BROKEN
	qDebug()<<g->CanonicalName();
#endif
	QList<ChainNarratorNode *> narrators;
	ChainNodeIterator itr=g->begin();
	for (;!itr.isFinished();++itr) {
		ChainNarratorNode & c=*itr;
		narrators.append(&c);
		c.setCorrespondingNarratorNodeGroup(NULL);
	}
	g->groupList.clear();
	NarratorGraph* graph=controller->getGraph();
	QList<NarratorNodeIfc *> new_nodes;
	int size=narrators.size();
	for (int j=0;j<size;j++) {
		new_nodes.append(narrators[j]);
	}
	for (int j=0;j<size;j++) {
		for (int k=j+1;k<size;k++) {
			double eq_val=equal(narrators[j]->getNarrator(),narrators[k]->getNarrator());
			if (eq_val>threshold) {
				NarratorNodeIfc * n1=&narrators[j]->getCorrespondingNarratorNode();
				NarratorNodeIfc * n2=&narrators[k]->getCorrespondingNarratorNode();
				NarratorNodeIfc * n_new=&graph->mergeNodes(*narrators[j],*narrators[k]);
				if (n1!=n_new && new_nodes.contains(n1))
					new_nodes.removeOne(n1);
				if (n2!=n_new && new_nodes.contains(n2))
					new_nodes.removeOne(n2);
				if (!new_nodes.contains(n_new))
					new_nodes.append(n_new);
			}
		}
	}
	//to keep all_nodes consistent
	graph->all_nodes.removeOne(g);
	graph->all_nodes.append(new_nodes);
	if (graph->top_nodes.contains(g)) {
		graph->top_nodes.removeOne(g);
		//check if still top both
		graph->top_nodes.append(new_nodes);
	}
	for (int i=0;i<new_nodes.size();i++)
		new_nodes[i]->color=g->color;
	if (!toDelete.contains(g))
		toDelete.append(g);
}

int deserializeGraph(QString fileName,ATMProgressIFC * prg) {
	fileName=fileName.split("\n")[0];
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
		return -1;
	QDataStream fileStream(&file);
	NarratorGraph *graph=new NarratorGraph(fileStream,prg);
	file.close();
#if 0
	QFile file2("graph2.dat");
	if (!file2.open(QIODevice::ReadWrite))
		return -1;
	QDataStream fileStream2(&file2);
	graph->serialize(fileStream2);
	file2.close();
	delete graph;
#else
	biographies(graph);
#endif
	return 0;
}

int mergeGraphs(QString fileName1,QString fileName2,ATMProgressIFC * prg) {
	fileName1=fileName1.split("\n")[0];
	QFile file1(fileName1);
	if (!file1.open(QIODevice::ReadOnly))
		return -1;
	QDataStream fileStream1(&file1);
	NarratorGraph *graph1=new NarratorGraph(fileStream1,prg);
	file1.close();
	fileName2=fileName2.split("\n")[0];
	QFile file2(fileName2);
	if (!file2.open(QIODevice::ReadOnly))
		return -1;
	QDataStream fileStream2(&file2);
	NarratorGraph *graph2=new NarratorGraph(fileStream2,prg);
	file2.close();

	graph1->mergeWith(graph2);
	delete graph2;
#if 1
	QFile file("graphMerged.dat");
	if (!file.open(QIODevice::ReadWrite))
		return -1;
	QDataStream fileStream(&file);
	graph1->serialize(fileStream);
	file.close();
	delete graph1;
#else
	biographies(graph1);
#endif
	return 0;
}
