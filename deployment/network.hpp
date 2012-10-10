/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of
 * the BSD license.
 *
 * See LICENSE and COPYING for more details.
 */

#ifndef NETWORK_HPP
#define	NETWORK_HPP

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>

#include "bitvector.hpp"

using namespace std;

class NetworkConnection;
class NetworkNode;

/**@brief (Deployment Application) a representation of a network domain as read from the configuration file.
 *
 * It contains all network nodes with their connections.
 *
 */
class Domain {
public:
    /**@brief constructor
     */
    Domain();
    /**@brief a vector containing all NetworkNode
     */
    vector<NetworkNode *> network_nodes;
    /**@brief a pointer to a NetworkNode that is the Rendezvous node of the domain.
     */
    NetworkNode *TM_node;
    /**@brief a pointer to a NetworkNode that is the TopologyManager of the domain.
     */
    NetworkNode *RV_node;
    /**@brief number of nodes in the domain.
     */
    unsigned int number_of_nodes;
    /**@brief number of connections in the domain.
     */
    unsigned int number_of_connections;
    /**@brief the length of an information identifier.
     */
    int ba_id_len;
    /**@brief the length of a LIPSIN Identifier.
     */
    int fid_len;
    /**@brief the full path of Click home.
     */
    string click_home;
    /**@brief the full path where configuration files will be written.
     */
    string write_conf;
    /**@brief the user using which deployment will remotely access network nodes.
     */
    string user;
    /**@brief whether sudo will be used when executing remote commands.
     */
    bool sudo;
    /*cinc: cache and popularity parameter*/
    int cache_degree ;
    int popthreshold ;
    int interval ;/*cinc: the popularity update frequency in sec*/
    /**@brief the overlay mode. mac or ip
     */
    string overlay_mode;
    /**@brief It prints an ugly representation of the Domain.
     */
    void printDomainData();
    /**@brief -
     *
     */
    void assignLIDs();
    /**@brief -
     *
     * @param LIDs
     * @param index
     */
    void calculateLID(vector<Bitvector> &LIDs, int index);
    /**@brief for each network node (and if the MAC address wasn't preassigned) it will ssh and learn the MAC address for all ethernet interfaces found in the configuration file.
     */
    void discoverMacAddresses();
    /**@brief returns the testbed IP address (dotted decimal string) given a node label.
     *
     * @param label a node label
     * @return the testbed IP address (dotted decimal string)
     */
    string getTestbedIPFromLabel(string label);
    /**@brief It locally creates and stores all Click/Blackadder configuration files for all network nodes (depending on the running mode).
     *
     * @param montoolstub generate monitor tool counter stub or not
     */
    void writeClickFiles(bool montoolstub);
    /**@brief Given a node label, it returns a pointer to the respective NetworkNode.
     *
     * @param label a node label.
     * @return a pointer to a NetworkNode.
     */
    NetworkNode *findNode(string label);
    /**@brief It copies the right Click/Blackadder configuration file to the right network node.
     */
    void scpClickFiles();
    /**@brief Copies the .graphml file to the right folder of the right network node.
     *
     * @param name the .graphml file name.
     */
    void scpTMConfiguration(string name);
    /**@brief Copies the tar gz file to all  network nodes and decompresses it at target home folder.
     *
     * @param tgzfile to get transferred and decompressed.
     */
    void scpClickBinary(string tgzfile);
    /**@brief It ssh'es network nodes, kills Click from user space and kernel space and runs Click in the correct mode (user ot kernel).
     */
    void startClick();
    /**@brief It starts the Topology Manager in the right network node (not implemented).
     */
    void startTM();
    /**@brief Writes the standard deployment config file format to a file.
     *
     * Writes the standard deployment config file format to a file. It is used along with barabasi albert model because the graph is autogenerated and the experiment can be repeated.
     *
     */
    string writeConfigFile(string filename);

};

class NetworkNode {
public:
    /***members****/
    string testbed_ip; //read from configuration file
    string label; //read from configuration file
    string running_mode; //user or kernel
    bool isRV; //read from configuration file
    bool isTM; //read from configuration file
    Bitvector iLid; //will be calculated
    Bitvector FID_to_RV; //will be calculated
    Bitvector FID_to_TM; //will be calculated
    vector<NetworkConnection *> connections;
    unsigned int cache_size ;//cinc: cache size of this node
};

class NetworkConnection {
public:
    /***members****/
    string src_label; //read from configuration file
    string dst_label; //read from configuration file

    string src_if; //read from configuration file /*e.g. tap0 or eth1*/
    string dst_if; //read from configuration file /*e.g. tap0 or eth1*/

    string src_ip; //read from configuration file /*an IP address - i will not resolve mac addresses in this case*/
    string dst_ip; //read from configuration file /*an IP address - i will not resolve mac addresses in this case*/

    string src_mac; //will be retrieved using ssh
    string dst_mac; //will be retrieved using ssh
    Bitvector LID; //will be calculated
};

#endif	/* NETWORK_HPP */
