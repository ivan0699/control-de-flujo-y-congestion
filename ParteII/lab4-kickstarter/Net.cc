#ifndef NET
#define NET

#include <string.h>
#include <omnetpp.h>
#include <packet_m.h>
#define N 8 // cantidad de nodos

using namespace omnetpp;

class Net: public cSimpleModule {
private:
    cOutVector sentPackets;
    cOutVector recPackets;
    cOutVector recFeedback;
    cOutVector hopCount;
    long Nodes[N];
public:
    Net();
    virtual ~Net();
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(Net);

#endif /* NET */

Net::Net() {
}

Net::~Net() {
}

void Net::initialize() {
    for (int i = 0; i < N; ++i){
        Nodes[i] = 0;
    }

    sentPackets.setName("Sent Packets");
    recPackets.setName("Recieved Packets");
    hopCount.setName("Hop Count");
    recFeedback.setName("FeedbackPkt Recieved");
}

void Net::finish() {
    recordScalar("Recieved Packets", hopCount.getValuesStored());
    recordScalar("Rerouted Packets", recPackets.getValuesStored());
}

void Net::handleMessage(cMessage *msg) {
    int current_node;
    Packet *pkt = (Packet*) msg;
    recPackets.record(1);

    if (pkt->getType() > 0){
        if((pkt->getSource() == this->getParentModule()->getIndex()) && !pkt->getDuplicated()){
            pkt->setDuplicated(true);
            send(pkt, "toLnk$o", pkt->getType() % 2);
        }
        else {
            Nodes[pkt->getSource()] = pkt->getUsedBuffer();
            delete(pkt);
        }
        return;
    }

    // If this node is the final destination, send to App
    if (pkt->getDestination() == this->getParentModule()->getIndex()) {
        hopCount.record(pkt->getHopCount());
        send(msg, "toApp$o");
    }
    // If not, forward the packet to some else... to who?
    else {
            bool destination;
            int mask = 1;

            //tamaños de los  buffers, de cada nodo
            Nodes[pkt->getSource()] = pkt->getUsedBuffer();
            current_node = this->getParentModule()->getIndex();
            int distance = current_node - pkt->getDestination();
            pkt->setHopCount(pkt->getHopCount() + 1);

            if(pkt->getHopCount() > 6){
                send(msg, "toLnk$o", 1);
            } else {
                if(current_node == 0 || current_node == 7){
                    destination = pkt->getDestination() < N/2;

                    if((Nodes[current_node-1] - Nodes[current_node+1]) > 100){
                        mask = 1;
                    }

                    else if((Nodes[current_node+1] - Nodes[current_node-1]) > 100){
                        mask = 0;
                    }

                    send(msg, "toLnk$o", destination&mask);
                }

                else {
                    destination = pkt->getDestination() > current_node;

                    if((Nodes[current_node-1] - Nodes[current_node+1]) > 100){
                        mask = 1;
                    }

                    else if((Nodes[current_node+1] - Nodes[current_node-1]) > 100){
                        mask = 0;
                    }

                    send(msg, "toLnk$o", destination&mask);
                }
            }

        sentPackets.record(1);

    }
}
