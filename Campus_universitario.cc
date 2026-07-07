/* =============================================================================
 * PROJETO EXPERIMENTAL - REDES DE COMPUTADORES
 * Mini-Mundo: Rede de um Campus Universitário
 *
 * Topologia (13 nós):
 *   - 1 Servidor Central (Data Center)
 *   - 1 Roteador Principal
 *   - 3 Switches (Lab1, Lab2, Secretaria) - Operando em Camada 2 (Bridge)
 *   - 3 PCs no Laboratório 1
 *   - 3 PCs no Laboratório 2
 *   - 2 PCs na Secretaria
 *
 * Modelo de Tráfego:
 *   - TCP/FTP : PCs dos laboratórios baixando arquivos do servidor
 *   - UDP/CBR : Secretaria simulando VoIP (tráfego constante)
 *
 * Simulador: NS-3
 * ============================================================================= */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/bridge-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CampusUniversitario");

int main(int argc, char *argv[])
{
    // ===================================================================
    // BLOCO 1: PARÂMETROS GLOBAIS E DE TRÁFEGO
    // ===================================================================
    Time::SetResolution(Time::NS);

    double tempoSimulacao = 20.0;
    uint32_t payloadFTP   = 1024;
    uint32_t payloadCBR   = 160;
    std::string taxaCBR   = "64kb/s";

    CommandLine cmd;
    cmd.AddValue("tempoSimulacao", "Tempo total da simulação", tempoSimulacao);
    cmd.AddValue("payloadFTP", "Tamanho do payload do FTP", payloadFTP);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadFTP));

    // ===================================================================
    // BLOCO 2: CRIAÇÃO DOS NÓS FÍSICOS (Equipamentos)
    // ===================================================================
    NodeContainer servidor; 
    servidor.Create(1);

    NodeContainer roteador; 
    roteador.Create(1);

    NodeContainer switchLab1, switchLab2, switchSecretaria;
    switchLab1.Create(1);
    switchLab2.Create(1);
    switchSecretaria.Create(1);

    NodeContainer pcLab1, pcLab2;
    pcLab1.Create(3);
    pcLab2.Create(3);

    NodeContainer pcSecretaria;
    pcSecretaria.Create(2);

    // ===================================================================
    // BLOCO 3: CARACTERÍSTICAS DOS CABOS (Velocidade e Atraso)
    // ===================================================================
    PointToPointHelper linkBackbone;
    linkBackbone.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    linkBackbone.SetChannelAttribute("Delay", StringValue("1ms"));

    CsmaHelper linkDistribuicao;
    linkDistribuicao.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    linkDistribuicao.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

    CsmaHelper linkLAN;
    linkLAN.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    linkLAN.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    // ===================================================================
    // BLOCO 4: CABEAMENTO E INTELIGÊNCIA DOS SWITCHES
    // ===================================================================
    NetDeviceContainer dispositivosBackbone = linkBackbone.Install(servidor.Get(0), roteador.Get(0));

    NodeContainer linkSw1(roteador.Get(0), switchLab1.Get(0));
    NetDeviceContainer devDistrib1 = linkDistribuicao.Install(linkSw1);

    NodeContainer linkSw2(roteador.Get(0), switchLab2.Get(0));
    NetDeviceContainer devDistrib2 = linkDistribuicao.Install(linkSw2);

    NodeContainer linkSwSec(roteador.Get(0), switchSecretaria.Get(0));
    NetDeviceContainer devDistribSec = linkDistribuicao.Install(linkSwSec);

    NodeContainer lanLab1; lanLab1.Add(switchLab1.Get(0)); lanLab1.Add(pcLab1);
    NetDeviceContainer devLAN1 = linkLAN.Install(lanLab1);

    NodeContainer lanLab2; lanLab2.Add(switchLab2.Get(0)); lanLab2.Add(pcLab2);
    NetDeviceContainer devLAN2 = linkLAN.Install(lanLab2);

    NodeContainer lanSec;  lanSec.Add(switchSecretaria.Get(0)); lanSec.Add(pcSecretaria);
    NetDeviceContainer devLANSec = linkLAN.Install(lanSec);

    BridgeHelper bridge;
    NetDeviceContainer portsSw1; portsSw1.Add(devDistrib1.Get(1)); portsSw1.Add(devLAN1.Get(0));
    bridge.Install(switchLab1.Get(0), portsSw1);

    NetDeviceContainer portsSw2; portsSw2.Add(devDistrib2.Get(1)); portsSw2.Add(devLAN2.Get(0));
    bridge.Install(switchLab2.Get(0), portsSw2);

    NetDeviceContainer portsSwSec; portsSwSec.Add(devDistribSec.Get(1)); portsSwSec.Add(devLANSec.Get(0));
    bridge.Install(switchSecretaria.Get(0), portsSwSec);

    // ===================================================================
    // BLOCO 5: INSTALAR TCP/IP (Apenas equipamentos Camada 3)
    // ===================================================================
    InternetStackHelper pilhaTCPIP;
    pilhaTCPIP.Install(servidor);
    pilhaTCPIP.Install(roteador);
    pilhaTCPIP.Install(pcLab1);
    pilhaTCPIP.Install(pcLab2);
    pilhaTCPIP.Install(pcSecretaria);

    // ===================================================================
    // BLOCO 6: DISTRIBUIÇÃO DE ENDEREÇOS IP E ROTEAMENTO
    // ===================================================================
    Ipv4AddressHelper ipv4;

    ipv4.SetBase("10.1.1.0", "255.255.255.252");
    Ipv4InterfaceContainer ifaceBackbone = ipv4.Assign(dispositivosBackbone);

    NetDeviceContainer redeLab1;
    redeLab1.Add(devDistrib1.Get(0)); redeLab1.Add(devLAN1.Get(1)); redeLab1.Add(devLAN1.Get(2)); redeLab1.Add(devLAN1.Get(3));
    ipv4.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceLab1 = ipv4.Assign(redeLab1);

    NetDeviceContainer redeLab2;
    redeLab2.Add(devDistrib2.Get(0)); redeLab2.Add(devLAN2.Get(1)); redeLab2.Add(devLAN2.Get(2)); redeLab2.Add(devLAN2.Get(3));
    ipv4.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceLab2 = ipv4.Assign(redeLab2);

    NetDeviceContainer redeSec;
    redeSec.Add(devDistribSec.Get(0)); redeSec.Add(devLANSec.Get(1)); redeSec.Add(devLANSec.Get(2));
    ipv4.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceSec = ipv4.Assign(redeSec);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // ===================================================================
    // BLOCO 7: APLICAÇÕES (O Tráfego da Rede)
    // ===================================================================
    
    uint16_t portaFTPLab1 = 21;
    PacketSinkHelper sinkFTPLab1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), portaFTPLab1));
    ApplicationContainer appSinkFTPLab1 = sinkFTPLab1.Install(pcLab1.Get(0));
    appSinkFTPLab1.Start(Seconds(1.0)); appSinkFTPLab1.Stop(Seconds(tempoSimulacao));

    BulkSendHelper ftpServidorLab1("ns3::TcpSocketFactory", InetSocketAddress(ifaceLab1.GetAddress(1), portaFTPLab1));
    ftpServidorLab1.SetAttribute("MaxBytes", UintegerValue(0)); 
    ApplicationContainer appSourceFTPLab1 = ftpServidorLab1.Install(servidor.Get(0));
    appSourceFTPLab1.Start(Seconds(2.0)); appSourceFTPLab1.Stop(Seconds(tempoSimulacao));

    uint16_t portaFTPLab2 = 22;
    PacketSinkHelper sinkFTPLab2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), portaFTPLab2));
    ApplicationContainer appSinkFTPLab2 = sinkFTPLab2.Install(pcLab2.Get(0));
    appSinkFTPLab2.Start(Seconds(1.5)); appSinkFTPLab2.Stop(Seconds(tempoSimulacao));

    BulkSendHelper ftpServidorLab2("ns3::TcpSocketFactory", InetSocketAddress(ifaceLab2.GetAddress(1), portaFTPLab2));
    ftpServidorLab2.SetAttribute("MaxBytes", UintegerValue(0)); 
    ApplicationContainer appSourceFTPLab2 = ftpServidorLab2.Install(servidor.Get(0));
    appSourceFTPLab2.Start(Seconds(2.5)); appSourceFTPLab2.Stop(Seconds(tempoSimulacao));

    uint16_t portaVoIP = 5000;
    PacketSinkHelper sinkVoIP("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), portaVoIP));
    ApplicationContainer appSinkVoIP = sinkVoIP.Install(pcSecretaria.Get(1));
    appSinkVoIP.Start(Seconds(1.0)); appSinkVoIP.Stop(Seconds(tempoSimulacao));

    OnOffHelper cbrVoIP("ns3::UdpSocketFactory", InetSocketAddress(ifaceSec.GetAddress(2), portaVoIP));
    cbrVoIP.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1000]"));
    cbrVoIP.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    cbrVoIP.SetAttribute("DataRate", DataRateValue(DataRate(taxaCBR)));
    cbrVoIP.SetAttribute("PacketSize", UintegerValue(payloadCBR));
    
    ApplicationContainer appSourceVoIP = cbrVoIP.Install(pcSecretaria.Get(0));
    appSourceVoIP.Start(Seconds(2.0)); appSourceVoIP.Stop(Seconds(tempoSimulacao));

    // ===================================================================
    // BLOCO 8: FLOWMONITOR E NETANIM (Coleta de Estatísticas e Visualização)
    // ===================================================================
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    AnimationInterface anim ("campus_network.xml");

    // Nomes Curtos
    anim.UpdateNodeDescription (servidor.Get(0), "Servidor");
    anim.UpdateNodeDescription (roteador.Get(0), "Roteador");
    
    anim.UpdateNodeDescription (switchLab1.Get(0), "SW-Lab1");
    anim.UpdateNodeDescription (switchLab2.Get(0), "SW-Lab2");
    anim.UpdateNodeDescription (switchSecretaria.Get(0), "SW-Sec");

    anim.UpdateNodeDescription (pcLab1.Get(0), "L1-PC1");
    anim.UpdateNodeDescription (pcLab1.Get(1), "L1-PC2");
    anim.UpdateNodeDescription (pcLab1.Get(2), "L1-PC3");

    anim.UpdateNodeDescription (pcLab2.Get(0), "L2-PC1");
    anim.UpdateNodeDescription (pcLab2.Get(1), "L2-PC2");
    anim.UpdateNodeDescription (pcLab2.Get(2), "L2-PC3");

    anim.UpdateNodeDescription (pcSecretaria.Get(0), "Sec-PC1");
    anim.UpdateNodeDescription (pcSecretaria.Get(1), "Sec-PC2");

    // Posições X,Y (Topologia em Árvore Alinhada)
  
    anim.SetConstantPosition (servidor.Get(0), 95.0, 10.0);
    anim.SetConstantPosition (roteador.Get(0), 95.0, 30.0);

    anim.SetConstantPosition (switchLab1.Get(0),       30.0, 60.0);
    anim.SetConstantPosition (switchLab2.Get(0),      100.0, 60.0);
    anim.SetConstantPosition (switchSecretaria.Get(0), 160.0, 60.0);

    anim.SetConstantPosition (pcLab1.Get(0), 10.0, 90.0);
    anim.SetConstantPosition (pcLab1.Get(1), 30.0, 90.0);
    anim.SetConstantPosition (pcLab1.Get(2), 50.0, 90.0);

    anim.SetConstantPosition (pcLab2.Get(0),  80.0, 90.0);
    anim.SetConstantPosition (pcLab2.Get(1), 100.0, 90.0);
    anim.SetConstantPosition (pcLab2.Get(2), 120.0, 90.0);

    anim.SetConstantPosition (pcSecretaria.Get(0), 150.0, 90.0);
    anim.SetConstantPosition (pcSecretaria.Get(1), 170.0, 90.0);
    // ===================================================================
    // BLOCO 9: EXECUÇÃO DO SIMULADOR
    // ===================================================================
    std::cout << "\nIniciando simulacao do Campus Universitario..." << std::endl;
    Simulator::Stop(Seconds(tempoSimulacao));
    Simulator::Run();

    // ===================================================================
    // BLOCO 10: IMPRESSÃO PROFISSIONAL DE RESULTADOS COM IPs
    // ===================================================================
    monitor->CheckForLostPackets(); 
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    std::cout << "\n========================================" << std::endl;
    std::cout << "        RESULTADOS DA SIMULACAO" << std::endl;
    std::cout << "========================================" << std::endl;

    int idFluxo = 1;
    for (auto it = stats.begin(); it != stats.end(); ++it)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);

        std::cout << "\n--- Fluxo " << idFluxo++ << " (" << t.sourceAddress << " -> " << t.destinationAddress << ") ---" << std::endl;
        std::cout << "  Pacotes enviados   : " << it->second.txPackets << std::endl;
        std::cout << "  Pacotes recebidos  : " << it->second.rxPackets << std::endl;
        std::cout << "  Pacotes perdidos   : " << it->second.lostPackets << std::endl;

        if (it->second.txPackets > 0) {
            double perdas = 100.0 * it->second.lostPackets / it->second.txPackets;
            std::cout << "  Taxa de perda      : " << perdas << " %" << std::endl;
        }

        if (it->second.rxBytes > 0) {
            double duracao = (it->second.timeLastRxPacket - it->second.timeFirstTxPacket).GetSeconds();
            double throughput = (it->second.rxBytes * 8.0) / duracao / 1000;
            std::cout << "  Throughput         : " << throughput << " Kbps" << std::endl;
        }

        if (it->second.rxPackets > 0) {
            double atrasoMedio = it->second.delaySum.GetSeconds() / it->second.rxPackets * 1000;
            std::cout << "  Atraso medio       : " << atrasoMedio << " ms" << std::endl;
        }
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "Simulacao concluida com sucesso!" << std::endl;
    std::cout << "Abra o arquivo 'campus_network.xml' no NetAnim." << std::endl;
    std::cout << "========================================\n" << std::endl;

    // ===================================================================
    // BLOCO 11: ENCERRAMENTO
    // ===================================================================
    Simulator::Destroy();
    return 0;
}
