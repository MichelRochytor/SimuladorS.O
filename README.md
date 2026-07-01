# Simulador de Escalonamento de Processos e Memória

Este projeto é um simulador didático de Sistemas Operacionais desenvolvido em **C** utilizando a biblioteca **Kosmos UI**. Ele permite visualizar de forma gráfica e em tempo real o comportamento de diferentes algoritmos de escalonamento de CPU e políticas de substituição de páginas na memória RAM.

---

## 📋 Funcionalidades

### 🔹 Algoritmos de Escalonamento de CPU
*   **Round-Robin (RR):** Escalonamento circular clássico com tempo de fatia de tempo (Quantum) configurável.
*   **SJF (Shortest Job First) Preemptivo:** Escolhe o processo que possui o menor tempo de execução restante.
*   **Prioridade Preemptiva:** Executa os processos com base no nível de prioridade (maior valor = maior prioridade).

### 🔹 Políticas de Substituição de Páginas (Memória)
*   **FIFO (First-In, First-Out):** Substitui a página que está há mais tempo na memória RAM.
*   **LRU (Least Recently Used):** Substitui a página que ficou mais tempo sem ser acessada.
*   **Random:** Algoritmo de escolha aleatória para substituição de quadros.

### 🔹 Interface e Métricas
*   **Gráfico de Gantt:** Renderização em tempo real dentro de um componente RichEdit, utilizando blocos coloridos para diferenciar os processos e destacar o tempo ocioso.
*   **Log Console:** Histórico detalhado informando o momento exato de cada execução e a ocorrência de *Page Faults*.
*   **Cálculo de Métricas:** Exibe automaticamente o tempo médio de resposta, tempo médio de espera e a quantidade total de falhas de página.

---

## 🛠 Pre-requisitos e Instalação

Para compilar e rodar este simulador, é necessário possuir a biblioteca **Kosmos UI** instalada no Windows.

1. Baixe e instale a ferramenta de gerenciamento através do link oficial:
   👉 **[Instalador Kosmos UI (GitHub)](https://github.com/MichelRochytor/KosmosUI-Installer)**

2. Certifique-se de que o executável `kosmos` foi adicionado corretamente às variáveis de ambiente do seu sistema.

---

## 🚀 Como Rodar o Projeto

Com o ambiente devidamente configurado, abra o seu terminal (CMD, PowerShell ou Bash) na pasta raiz deste projeto e execute o comando:

```bash
kosmos run SimuladorS.O