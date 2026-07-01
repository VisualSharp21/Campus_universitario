#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/bridge-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

int main (int argc, char *argv[]) {

    //Bloco 1
    double tempoSimulacao = 20.0;
    uint32_t payloadFTP = 1024;
    uint32_t payloadCBR = 160;
    std::string taxaCBR = "64kb/s";

    Time::SetResolution (Time::NS);
    CommandLine cmd;
    cmd.AddValue ("TempoSimulacao", "Tempo real da simulação", tempoSimulacao);
    cmd.AddValue ("payloadFTP", "Tamanho real do payload do FTP", payloadFTP);
    cmd.Parse (argc, argv);

    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadFTP));


    //Bloco 2
    NodeContainer centralServer;
    centralServer.Create(1);

    NodeContainer mainRouter;
    mainRouter.Create(1);

    NodeContainer switchLab1;
    switchLab1.Create(1);

    NodeContainer switchLab2;
    switchLab2.Create(1);

    NodeContainer switchSecretaria;
    switchSecretaria.Create(1);

    NodeContainer pcsLab1;
    pcsLab1.Create(3);

    NodeContainer pcsLab2;
    pcsLab2.Create(3);

    NodeContainer pcsSecretaria;
    pcsSecretaria.Create(2);

    //Bloco 3 definição dos tipos de link
    //link do servidor central para o rot principal e vice versa
    PointToPointHelper p2pBackone;
    p2pBackone.SetDeviceAttribute ("DateRate", StringValue("1Gbps"));
    p2pBackone.SetChannelAttribute("Delay",StringValue("1ms"));

    //Link de distribuição. Rot principal para os switches e vice versa
    PointToPointHelper p2pDistruicao;
    p2pDistruicao.SetDeviceAttribute("DateRate", StringValue("100Mbps"));
    p2pDistruicao.SetChannelAttribute("Delay", StringValue("2ms"));

    //link das redes LAN. Switches para PCs e vice versa
    CsmaHelper csmaLan;
    csmaLan.SetChannelAttribute ("DataRate", StringValue("100Mbps"));
    csmaLan.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    //Bloco 4 conexão dos nós com os links
    //Instala 1 placa P2P de 1Gps no Servidor e 1 no Roteador
    NetDeviceContainer devicesBackbone;
    devicesBackbone = p2pBackone.Install (centralServer.Get(0), mainRouter.Get(0));

    //Conecta a Distribuição (roteador <-> switches) via p2p de 100Mbps
    NetDeviceContainer devicesRouterSw1 = p2pDistruicao.Install (mainRouter.Get(0), switchLab1.Get(0));
    NetDeviceContainer devicesRouterSw2 = p2pDistruicao.Install (mainRouter.Get(0), switchLab2.Get(0));
    NetDeviceContainer devicesRouterSw3 = p2pDistruicao.Install (mainRouter.Get(0), switchSecretaria.Get(0));

    //Cria as redes locais via CSMA
    //Agrupa o switch e os PCs do mesmo setor antes de passar o cabo CSMA

    //LAN Lab1
    NodeContainer lanLab1;
    lanLab1.Add (switchLab1.Get(0)); //Adiciona o switch do Lab 1 na rede local
    lanLab1.Add (pcsLab1); //Adiciona os 3 PCs do lab1
    NetDeviceContainer devicesLanLab1 = csmaLan.Install(lanLab1);

    //LAN Lab2
    NodeContainer lanLab2;
    lanLab2.Add (switchLab2.Get(0)); 
    lanLab2.Add (pcsLab2);
    NetDeviceContainer devicesLanLab2 = csmaLan.Install(lanLab2);

    // LAN Secretaria
    NodeContainer lanSecretaria;
    lanSecretaria.Add (switchSecretaria.Get(0)); 
    lanSecretaria.Add (pcsSecretaria);           
    NetDeviceContainer devicesLanSecretaria = csmaLan.Install (lanSecretaria);

    //Faz os nós dos Switches operarem como camada 2
    BridgeHelper bridge;

    //Switch do Lab 1. Vai unir a porta que vem do roteador com a porta da rede local
    NetDeviceContainer portsSw1;
    portsSw1.Add (devicesRouterSw1.Get(1)); // Índice 1 é a ponta do Switch (0 é o Roteador)
    portsSw1.Add (devicesLanLab1.Get(0)); // Índice 0 é a porta CSMA do Switch
    bridge.Install (switchLab1.Get(0), portsSw1);

    //Switch do Lab 2
    NetDeviceContainer portsSw2;
    portsSw2.Add (devicesRouterSw2.Get(1)); 
    portsSw2.Add (devicesLanLab2.Get(0));   
    bridge.Install (switchLab2.Get(0), portsSw2);

    // Switch da Secretaria
    NetDeviceContainer portsSw3;
    portsSw3.Add (devicesRouterSw3.Get(1)); 
    portsSw3.Add (devicesLanSecretaria.Get(0));   
    bridge.Install (switchSecretaria.Get(0), portsSw3);



    
    // bloco 5 Instalação do TCP/IP (camada 3)
    InternetStackHelper stack;
    
    // para instalar a pilha de internet no Servidor, Roteador e PCs
    stack.Install (centralServer);
    stack.Install (mainRouter);
    stack.Install (pcsLab1);
    stack.Install (pcsLab2);
    stack.Install (pcsSecretaria);
    
    
    // Bloco 6. Distruição do endereços IP por Sub-rede 
    Ipv4AddressHelper address;

    // Sub-rede do Backbone (Servidor <-> Roteador)
    // Usamos uma máscara /30 (255.255.255.252) que é a melhor prática para links Ponto a Ponto (apenas 2 IPs úteis)
    address.SetBase ("10.1.1.0", "255.255.255.252");
    Ipv4InterfaceContainer interfacesBackbone = address.Assign (devicesBackbone);

    // Sub-rede do Lab 1 (192.168.1.0/24)
    // Agrupamos a porta do roteador e os PCs do Lab 1 para receberem IPs da mesma família
    NetDeviceContainer redeLab1;
    redeLab1.Add (devicesRouterSw1.Get(0)); // Índice 0 é a porta do Roteador voltada pro Lab 1
    redeLab1.Add (devicesLanLab1);          // Contém o Switch e os 3 PCs
    
    address.SetBase ("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfacesLab1 = address.Assign (redeLab1);

    //Sub-rede do Lab 2 (192.168.2.0/24)
    NetDeviceContainer redeLab2;
    redeLab2.Add (devicesRouterSw2.Get(0)); 
    redeLab2.Add (devicesLanLab2);          
    
    address.SetBase ("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfacesLab2 = address.Assign (redeLab2);

    // 6.4. Sub-rede da Secretaria (192.168.3.0/24)
    NetDeviceContainer redeSecretaria;
    redeSecretaria.Add (devicesRouterSw3.Get(0)); 
    redeSecretaria.Add (devicesLanSecretaria);          
    
    address.SetBase ("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interfacesSecretaria = address.Assign (redeSecretaria);

    // Tabelas de roteamento
    // Sem isso, os PCs do Lab 1 (rede 192.168.1.x) não fariam ideia de como chegar
    // no Servidor (rede 10.1.1.x). Isso gera as tabelas de roteamento dinamicamente
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();



}
