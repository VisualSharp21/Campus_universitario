# Simulação de Rede de Campus Universitário — NS-3

Projeto de simulação de uma infraestrutura de rede de campus utilizando o **NS-3**, desenvolvido para estudar comportamento de tráfego e métricas de desempenho de redes.

## Objetivo

Modelar uma topologia de rede e avaliar seu desempenho por meio de simulação, observando métricas como:

- throughput;
- pacotes transmitidos e recebidos;
- comportamento dos fluxos;
- desempenho das conexões.

## Tecnologias

- C++
- NS-3
- Flow Monitor
- NetAnim
- redes de computadores
- simulação de redes

## Recursos utilizados

O projeto utiliza módulos do NS-3 como:

- `network-module`
- `internet-module`
- `point-to-point-module`
- `flow-monitor-module`
- `netanim-module`

## Avaliação de desempenho

O código utiliza o Flow Monitor para coletar estatísticas dos fluxos da simulação. O throughput é calculado a partir da quantidade de bytes recebidos e do intervalo efetivo de transmissão.

```text
Topologia NS-3
      ↓
Geração de tráfego
      ↓
Flow Monitor
      ↓
Métricas de desempenho
      ↓
Throughput e análise dos fluxos
```

## Arquivos

- `Campus_universitario.cc`
- `Campus_universitariom.cc`

## O que este projeto demonstra

- modelagem de redes;
- programação C++;
- simulação de sistemas de comunicação;
- coleta e interpretação de métricas;
- análise experimental de desempenho.
