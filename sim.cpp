#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iterator>
#include <string>
#include <fstream>
#include <sstream>

struct node{
    int id;
    std::vector<int> neighbors;

    std::vector<int> dest;
    std::vector<int> cost;
    std::vector<int> nextHop;
    int change_flag;

    struct packet* outgoing_DV_packets;
    struct packet* incoming_DV_packets;
    struct packet_to_route* packet_being_routed;
    struct node* next;
};

struct packet{
    int source_node;
    int dest_node;
    std::vector<int> dest;
    std::vector<int> cost;

    struct packet* next;
};

struct packet_to_route{
    int source_node;
    int dest_node;
    int next_node;
};

struct node* create_node(int id, int neighbor, int cost);
void update_node(struct node* update, int neighbor, int cost);
void init(std::string filename);
void create_DV_packets();
void send_DV_packets();
void route_packet(int source, int dest);

struct node* head;
bool routing_table_change;
int DV_packet_count;

struct node* create_node(int id, int neighbor, int cost){
    //create new node and initialize id, neighbor, and routing table
    struct node* new_node = new node;
    new_node->id = id;
    new_node->neighbors.push_back(neighbor);
    new_node->change_flag = 0;

    new_node->dest.push_back(neighbor);
    new_node->cost.push_back(cost);
    new_node->nextHop.push_back(neighbor);

    new_node->outgoing_DV_packets = NULL;
    new_node->incoming_DV_packets = NULL;
    new_node->next = NULL;

    return new_node;
}

void update_node(struct node* update, int neighbor, int cost){
    update->neighbors.push_back(neighbor);

    update->dest.push_back(neighbor);
    update->cost.push_back(cost);
    update->nextHop.push_back(neighbor);
}

struct node* locate_node(struct node* head, int id){
    struct node* temp = head;

    while(temp != NULL && temp->id != id){
        temp = temp->next;
    }

    return temp;
}

void init(std::string filename){
    std::string txt;
    int node1;
    int node2;
    int cost;
    std::string temp;

    DV_packet_count = 0;
    head = NULL;

    std::ifstream top_file(filename.c_str());
    if(!top_file) {std::cout << "Error opening file.\n" << std::endl; exit(1);}

    while(getline(top_file, txt))
    {
        std::stringstream s(txt);

        getline(s, temp, '\t');
        node1 = atoi(temp.c_str());

        getline(s, temp, '\t');
        node2 = atoi(temp.c_str());

        getline(s, temp, '\t');
        cost = atoi(temp.c_str());

        //std::cout << node1 << " " << node2 << " " << cost << std::endl;

        if(head == NULL){
            head = create_node(node1, node2, cost);
            head->next = create_node(node2, node1, cost);
        }else{
            struct node* curr = head;
            while(curr->id != node1 && curr->next != NULL){
                curr = curr->next;
            }

            if(curr->id != node1){
                curr->next = create_node(node1, node2, cost);
            }
            else{
                update_node(curr, node2, cost);
            }

            curr = head;
            while(curr->id != node2 && curr->next != NULL){
                curr = curr->next;
            }

            if(curr->id != node2){
                curr->next = create_node(node2, node1, cost);
            }
            else{
                update_node(curr, node1, cost);
            }
        }
    }
}

void create_DV_packets()
{
    node* curr = head;

    while(curr != NULL)
    {
        for(int i = 0; i < curr->neighbors.size(); i++)
        {
            packet* new_dv_packet = new packet;
            new_dv_packet->source_node = curr->id;
            new_dv_packet->dest_node = curr->neighbors.at(i);

            for(int j = 0; j < curr->dest.size(); j++)
            {
                if(curr->nextHop.at(j) == new_dv_packet->dest_node || curr->dest.at(j) == new_dv_packet->dest_node)
                {
                    //do nothing
                }else{
                    new_dv_packet->dest.push_back(curr->dest.at(j));
                    new_dv_packet->cost.push_back(curr->cost.at(j));
                }
            }

            new_dv_packet->next = curr->outgoing_DV_packets;
            curr->outgoing_DV_packets = new_dv_packet;

            DV_packet_count += 1;
        }

        curr = curr->next;
    }
}

void send_DV_packets()
{
    node* curr = head;
    packet* packet_in_transit;

    while(curr != NULL)
    {
        while(curr->outgoing_DV_packets != NULL)
        {
            packet_in_transit = curr->outgoing_DV_packets;
            int destination = packet_in_transit->dest_node;
            curr->outgoing_DV_packets = curr->outgoing_DV_packets->next;
            packet_in_transit->next = NULL;

            node* find_destination_node = head;
            while(find_destination_node != NULL) {
                if (find_destination_node->id == destination) {
                    packet_in_transit->next = find_destination_node->incoming_DV_packets;
                    find_destination_node->incoming_DV_packets = packet_in_transit;
                }
                find_destination_node = find_destination_node->next;
            }
        }

        curr = curr->next;
    }
}

void update_routing_tables()
{
    node* curr = head;

    while(curr != NULL)
    {
        while(curr->incoming_DV_packets != NULL)
        {
            int advertiser = curr->incoming_DV_packets->source_node;
            int advertiser_cost = 0;
            for(int i = 0; i < curr->dest.size(); i++)
            {
                if(curr->dest.at(i) == advertiser)
                    advertiser_cost = curr->cost.at(i);
            }

            for(int i = 0; i < curr->incoming_DV_packets->dest.size(); i++)
            {
                int destination = curr->incoming_DV_packets->dest.at(i);
                int cost = curr->incoming_DV_packets->cost.at(i);
                bool found = false;

                for(int j = 0; j < curr->dest.size(); j++)
                {
                    if(curr->dest.at(j) == destination)
                    {
                        found = true;
                        if((cost + advertiser_cost) < curr->cost.at(j)){
                            curr->cost.at(j) = cost + advertiser_cost;
                            curr->nextHop.at(j) = curr->incoming_DV_packets->source_node;
                            curr->change_flag = 1;
                        }
                    }
                }

                if(!found)
                {
                    curr->dest.push_back(destination);
                    curr->cost.push_back(cost + advertiser_cost);
                    curr->nextHop.push_back(curr->incoming_DV_packets->source_node);
                    curr->change_flag = 1;
                }
            }

            curr->incoming_DV_packets = curr->incoming_DV_packets->next;
        }

        curr = curr->next;
    }
}

void route_packet(int source, int dest)
{
    packet_to_route* packet_in_transit = new packet_to_route;
    packet_in_transit->dest_node = dest;
    packet_in_transit->source_node = source;

    int routing_complete = 0;
    packet_to_route* router;

    node* curr = head;
    while(curr != NULL)
    {
        if (curr->id == packet_in_transit->source_node){
            curr->packet_being_routed = packet_in_transit;
            break;
        }
        curr = curr->next;
    }

    std::cout << curr->id << " ";

    for(int i = 0; i < curr->dest.size(); i++)
    {
        if (curr->dest.at(i) == packet_in_transit->dest_node)
        {
            packet_in_transit->next_node = curr->nextHop.at(i);
        }
    }

    while(!routing_complete)
    {
        router = packet_in_transit;
        curr->packet_being_routed = NULL;
        curr = head;
        while(curr != NULL)
        {
            if (curr->id == router->next_node){
                curr->packet_being_routed = packet_in_transit;
                router = NULL;
                break;
            }
            curr = curr->next;
        }

        if(curr->id == curr->packet_being_routed->dest_node){
            routing_complete = 1;
        }else{
            for(int i = 0; i < curr->dest.size(); i++)
            {
                if (curr->dest.at(i) == curr->packet_being_routed->dest_node)
                {
                    curr->packet_being_routed->next_node = curr->nextHop.at(i);
                }
            }
        }

        std::cout << curr->id << " ";
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::string filename;
    std::string txt;

    if (argc != 2) {fprintf(stderr, "USAGE: %s input_file\n", argv[0]); exit(-1);}
    filename = argv[1];
    init(filename);

    routing_table_change = true;
    int count = 0;
    int previous_round_DV_count = 0;
    int last_to_converge;

    while(routing_table_change)
    {
        previous_round_DV_count = DV_packet_count;
        count++;
        routing_table_change = false;
        create_DV_packets();
        send_DV_packets();
        update_routing_tables();

        node* curr = head;
        while(curr != NULL)
        {
            if(curr->change_flag == 1)
            {
                curr->change_flag = 0;
                routing_table_change = true;
		        last_to_converge = curr->id;
                //std::cout << "Node " << curr->id << "\t changed in round " << count << std::endl;
            }
            curr = curr->next;
        }
    }


    node* print = head;
    while(print != NULL){
        std::cout << "ID: " << print->id << std::endl;
        std::cout << "Neighbors: ";
        for(int i = 0; i < print->neighbors.size(); i++)
		    std::cout << print->neighbors.at(i) << " ";
        std::cout << "\n" << std::endl;

        std::cout << "Dest:    ";
        for(int i = 0; i < print->dest.size(); i++)
                std::cout << print->dest.at(i) << "\t";
        std::cout << std::endl;

        std::cout << "Cost:    ";
        for(int i = 0; i < print->cost.size(); i++)
                std::cout << print->cost.at(i) << "\t";
        std::cout << std::endl;

        std::cout << "NextHop: ";
        for(int i = 0; i < print->nextHop.size(); i++)
                std::cout << print->nextHop.at(i) << "\t";
        std::cout << "\n" << std::endl;

        packet* print_packet = print->incoming_DV_packets;
        while(print_packet != NULL){
            std::cout << "Dest_node: " << print_packet->dest_node << std::endl;
            std::cout << "Source_node: " << print_packet->source_node << std::endl;

            std::cout << "Dest:    ";
            for(int i = 0; i < print_packet->dest.size(); i++)
                std::cout << print_packet->dest.at(i) << "\t";
            std::cout << std::endl;

            std::cout << "Cost:    ";
            for(int i = 0; i < print_packet->cost.size(); i++)
                std::cout << print_packet->cost.at(i) << "\t";
            std::cout << "\n" << std::endl;

            print_packet = print_packet->next;
        }

        print = print->next;
    }

    std::cout << "Rounds to converge: " << count - 1 << std::endl;
    std::cout << "Number of DV packets created: " << previous_round_DV_count << std::endl;
    std::cout << "Last node to converge: " << last_to_converge << std::endl;

    if(filename == "topology1.txt"){
	std::cout << "Path for packet with source 0 and destination 3: ";
        route_packet(0, 3);
    }
    else if(filename == "topology2.txt"){
        std::cout << "Path for packet with source 0 and destination 7: ";
        route_packet(0, 7);
    }
    else if(filename == "topology3.txt"){
        std::cout << "Path for packet with source 0 and destination 23: ";
        route_packet(0, 23);
    }

    return 0;
}
