# Proj1 — Processamento de Imagens

**Disciplina:** Computação Visual  
**Professor:** André Kishimoto  
**Universidade:** Universidade Presbiteriana Mackenzie — FCI  

---

## Integrantes do Grupo

| Matheus Alonso Varjao | 10417888 |
| (Victor Maki) | 10419861 |
| Leonardo Moreira | 10417555 |
| # | Nome | RA |
|---|---|---|
| 1 | Leonardo Moreira | 10417555 |
| 2 | Victor Maki Tarcha | 10419861 |
| 3 | Matheus Alonso Varjao | 10417888 |

---

## Descrição do Projeto

Software de processamento de imagens desenvolvido em **C++17** utilizando as bibliotecas **SDL3**, **SDL3_image** e **SDL3_ttf**. O programa abre uma imagem via linha de comando, converte-a para escala de cinza, exibe um histograma interativo e permite equalizar/reverter o histograma com um clique de botão.

---

## Funcionalidades

| # | Funcionalidade | Descrição |
|---|---|---|
| 1 | **Carregamento de imagem** | Suporta PNG, JPG e BMP; trata erros de arquivo não encontrado ou formato inválido |
| 2 | **Conversão para escala de cinza** | Detecta se a imagem já é cinza; caso contrário aplica `Y = 0.2125R + 0.7154G + 0.0721B` |
| 3 | **GUI com duas janelas** | Janela principal (imagem) e janela secundária filha (histograma + botão) |
| 4 | **Histograma** | Exibe histograma proporcional, média (clara/média/escura) e desvio padrão (alto/médio/baixo) |
| 5 | **Equalização** | Botão alterna entre imagem equalizada e original; histograma e janela principal atualizam automaticamente |
| 6 | **Salvar imagem** | Tecla **S** salva a imagem atual como `output_image.png` (sobrescreve se existir) |

---

## Requisitos

- **Compilador:** g++ 15.1.0 (C++17)
- **Bibliotecas:** SDL3, SDL3_image, SDL3_ttf
- **Sistema:** Linux, macOS ou Windows (com as libs instaladas)

### Instalando as dependências (Ubuntu/Debian)

```bash
# SDL3 ainda não está nos repos oficiais do Ubuntu — instale via fonte ou PPA
# Exemplo rápido para sistemas com SDL3 disponível via pkg-config:
sudo apt install libsdl3-dev libsdl3-image-dev libsdl3-ttf-dev
```

### Instalando as dependências (Arch Linux)

```bash
sudo pacman -S sdl3 sdl3_image sdl3_ttf
```

### Instalando as dependências (macOS com Homebrew)

```bash
brew install sdl3 sdl3_image sdl3_ttf
```

---

## Compilação

```bash
# Na raiz do repositório (onde está o Makefile):
make
```

Isso gera o executável `programa` na pasta raiz.

Para limpar os arquivos gerados:

```bash
make clean
```

---

## Execução

```bash
./programa caminho_da_imagem.ext
```

Exemplos:

```bash
./programa imagens/foto.jpg
./programa imagens/teste.png
./programa imagens/exemplo.bmp
```

---

## Controles

| Ação | Como fazer |
|---|---|
| Equalizar / Ver original | Clicar no botão azul na janela secundária |
| Salvar imagem atual | Pressionar a tecla **S** |
| Fechar o programa | Fechar qualquer janela ou pressionar **ESC** |

---

## Como o Programa Funciona

1. **Carregamento:** A imagem é carregada com `IMG_Load` da SDL3_image.
2. **Detecção de cor:** Cada pixel é verificado; se R ≠ G ou G ≠ B em algum pixel, a imagem é considerada colorida.
3. **Conversão:** A fórmula de luminância `Y = 0.2125R + 0.7154G + 0.0721B` é aplicada pixel a pixel.
4. **Histograma:** Contagem de frequência de cada nível de cinza (0–255).
5. **Análise:** Média e desvio padrão calculados a partir do histograma para classificar brilho e contraste.
6. **Equalização:** Usa a função de distribuição cumulativa (CDF) do histograma para redistribuir os níveis de cinza uniformemente.
7. **Renderização:** SDL3 renderiza a imagem na janela principal e o histograma/botão na janela secundária a cada frame.

---

## Contribuições dos Integrantes

## Contribuições dos Integrantes

### 1. Leonardo Moreira dos Santos (RA: 10417555)
**Responsável pelo Núcleo de Processamento de Imagens:**

* [cite_start]**Conversão de Espaço de Cor:** Implementação do algoritmo de conversão de imagens coloridas para escala de cinza utilizando a fórmula de luminância: $Y = 0.2125R + 0.7154G + 0.0721B$. 
* [cite_start]**Análise Estatística de Dados:** Criação das rotinas de contagem de frequência (histograma) e cálculo de métricas estatísticas como Média de Intensidade e Desvio Padrão. 
* [cite_start]**Algoritmo de Equalização:** Desenvolvimento do mecanismo de equalização global baseado na Função de Distribuição Cumulativa (CDF) para redistribuição de tons.
* [cite_start]**Gestão de Memória e Superfícies:** Implementação de utilitários para conversão de formatos de pixels (RGBA8888) e rotinas de exportação de arquivos para o formato PNG. 

**Principais Funções Desenvolvidas:**
* [cite_start]`convertToGray`: Padronização de imagens PNG/JPG/BMP para a base de processamento do projeto. 
* [cite_start]`analyzeHistogram`: Motor de classificação automática de brilho (clara/média/escura) e contraste. 
* [cite_start]`equalizeHistogram`: Funcionalidade central para expansão de histograma e melhoria visual. 

---

### 2. Matheus Alonso Varjao (RA: 10417888)
**Responsável pela Interface Gráfica e Visualização (GUI):**

* [cite_start]**Arquitetura de Janelas:** Desenvolvimento do sistema de janelas múltiplas utilizando a biblioteca **SDL3**, garantindo que a janela secundária seja criada como dependente (filha) da principal.
* [cite_start]**Visualização de Dados:** Implementação da renderização gráfica do histograma de forma proporcional e clara na janela secundária. 
* [cite_start]**Sistema de UI e Primitivas:** Criação de componentes visuais (botões interativos) desenhados com primitivas SDL, incluindo a gestão de estados visuais (Normal, Hover e Pressionado). 
* [cite_start]**Integração de Texto:** Implementação da exibição dinâmica das métricas de processamento na tela através da biblioteca `SDL_ttf`. 


**Principais Funções Desenvolvidas:**
* [cite_start]`renderSecondary`: Gestão completa da janela de histograma e informações de análise. 
* [cite_start]`drawButton`: Lógica visual e interativa dos componentes de interface. 
* [cite_start]`renderText`: Utilitário para renderização de fontes e dados técnicos em tempo real. 

---

### 3. Victor Maki Tarcha (RA: 10419861)
**Responsável pela Interatividade e Controle de Fluxo:**

* [cite_start]**Gestão de Eventos:** Implementação do loop principal de eventos, tratando entradas de teclado (tecla **S** para salvar) e mouse para interação com a GUI. 
* [cite_start]**Lógica de Alternância (Toggle):** Desenvolvimento da lógica de controle que permite alternar entre a visualização original em cinza e a versão equalizada sem recarregar o arquivo. 
* [cite_start]**Sincronização de Renderização:** Garantia de que as atualizações de processamento (equalização) sejam refletidas simultaneamente em ambas as janelas. 




---

## Estrutura do Repositório

```
proj1/
├── src/
│   └── main.cpp       # Código-fonte principal
├── Makefile           # Script de compilação
└── README.md          # Esta documentação
```

> **Nota:** Arquivos intermediários de compilação (`.o`, executável) **não** estão no repositório (ver `.gitignore`).
