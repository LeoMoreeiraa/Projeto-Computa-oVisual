/*
 * Projeto 1 - Processamento de Imagens
 * Disciplina: Computação Visual
 * Universidade Presbiteriana Mackenzie
 * Professor: André Kishimoto
 *
 * Grupo:  Leonardo Moreira - 10417555 
 * Victor Maki Tarcha - 10419861,
 * Matheus Alonso Varjao - 10417888
 */
 
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
 
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
 
// ─── Constantes ───────────────────────────────────────────────────────────────
 
static const int   SEC_WIN_W      = 320;   // Largura da janela secundária
static const int   SEC_WIN_H      = 520;   // Altura da janela secundária
static const int   HIST_MARGIN    = 20;   // Margem interna do histograma
static const int   HIST_H         = 180;  // Altura do gráfico do histograma
static const int   FONT_SIZE      = 14;   // Tamanho da fonte do texto
static const int   BTN_H          = 36;   // Altura do botão
static const int   BTN_W          = 200;  // Largura do botão
static const float LUM_DARK_TH    = 85.0f;  // Limiar para imagem escura
static const float LUM_BRIGHT_TH  = 170.0f; // Limiar para imagem clara
static const float STD_LOW_TH     = 40.0f;  // Limiar para baixo contraste
static const float STD_HIGH_TH    = 80.0f;  // Limiar para alto contraste
 
// ─── Cores dos estados do botão ───────────────────────────────────────────────
 
static const SDL_Color BTN_NORMAL  = {  66, 135, 245, 255 }; // Azul padrão
static const SDL_Color BTN_HOVER   = { 130, 185, 255, 255 }; // Azul claro (mouse sobre)
static const SDL_Color BTN_PRESSED = {  30,  80, 180, 255 }; // Azul escuro (clicado)
 
// ─── Funções Auxiliares ───────────────────────────────────────────────────────
 
// Garante que o valor de intensidade esteja no intervalo [0, 255]
static Uint8 clampU8(float v) {
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return (Uint8)v;
}
 
// ─── Utilidades de Imagem ─────────────────────────────────────────────────────
 
// Converte uma superfície para o formato RGBA de 32 bits
static SDL_Surface* toRGBA(SDL_Surface* src) {
    return SDL_ConvertSurface(src, SDL_PIXELFORMAT_RGBA8888);
}
 
// Verifica se a imagem já está em escala de cinza (R=G=B em todos os pixels)
static bool isGrayscale(SDL_Surface* surf) {
    SDL_Surface* rgba = toRGBA(surf);
    if (!rgba) return false;
    bool gray = true;
    Uint8* px = (Uint8*)rgba->pixels;
    for (int i = 0; i < rgba->w * rgba->h && gray; ++i) {
        Uint8 r = px[i * 4 + 0];
        Uint8 g = px[i * 4 + 1];
        Uint8 b = px[i * 4 + 2];
        if (r != g || g != b) gray = false;
    }
    SDL_DestroySurface(rgba);
    return gray;
}
 
// Converte imagem colorida para cinza usando a fórmula de luminância [cite: 59]
static SDL_Surface* convertToGray(SDL_Surface* src) {
    SDL_Surface* rgba = toRGBA(src);
    if (!rgba) return nullptr;
    int w = rgba->w, h = rgba->h;
    Uint8* px = (Uint8*)rgba->pixels;
    for (int i = 0; i < w * h; ++i) {
        Uint8 r = px[i * 4 + 0];
        Uint8 g = px[i * 4 + 1];
        Uint8 b = px[i * 4 + 2];
        // Cálculo ponderado: 21.25% R + 71.54% G + 07.21% B [cite: 59]
        Uint8 y = clampU8(0.2125f * r + 0.7154f * g + 0.0721f * b);
        px[i * 4 + 0] = y;
        px[i * 4 + 1] = y;
        px[i * 4 + 2] = y;
    }
    return rgba;
}
 
// Conta a frequência de cada nível de cinza (0-255)
static void computeHistogram(SDL_Surface* surf, int hist[256]) {
    memset(hist, 0, 256 * sizeof(int));
    Uint8* px = (Uint8*)surf->pixels;
    int n = surf->w * surf->h;
    for (int i = 0; i < n; ++i)
        hist[px[i * 4]]++;
}
 
// Calcula a média e o desvio padrão para análise de brilho e contraste [cite: 69, 70]
static void analyzeHistogram(const int hist[256], int totalPixels,
                              float& meanOut, float& stdOut) {
    double sum = 0, sumSq = 0;
    for (int v = 0; v < 256; ++v) {
        sum   += (double)v * hist[v];
        sumSq += (double)v * v * hist[v];
    }
    meanOut = (float)(sum / totalPixels);
    float variance = (float)(sumSq / totalPixels - (sum / totalPixels) * (sum / totalPixels));
    stdOut = sqrtf(variance < 0 ? 0 : variance);
}
 
// Aplica a equalização global baseada na função de distribuição cumulativa (CDF) [cite: 74]
static SDL_Surface* equalizeHistogram(SDL_Surface* gray) {
    int hist[256];
    computeHistogram(gray, hist);
    int n = gray->w * gray->h;
 
    int cdf[256] = {};
    cdf[0] = hist[0];
    for (int i = 1; i < 256; ++i) cdf[i] = cdf[i - 1] + hist[i];
 
    int cdfMin = 0;
    for (int i = 0; i < 256; ++i) { if (cdf[i] > 0) { cdfMin = cdf[i]; break; } }
 
    Uint8 lut[256]; // Tabela de consulta (Look-up Table) para mapeamento
    for (int v = 0; v < 256; ++v) {
        if (n == cdfMin)
            lut[v] = v;
        else
            lut[v] = clampU8((float)(cdf[v] - cdfMin) / (n - cdfMin) * 255.0f);
    }
 
    SDL_Surface* out = toRGBA(gray);
    Uint8* px = (Uint8*)out->pixels;
    for (int i = 0; i < n; ++i) {
        Uint8 y = lut[px[i * 4]];
        px[i * 4 + 0] = y;
        px[i * 4 + 1] = y;
        px[i * 4 + 2] = y;
    }
    return out;
}
 
// ─── Renderização de Texto ────────────────────────────────────────────────────
 
static void renderText(SDL_Renderer* ren, TTF_Font* font,
                       const char* text, int x, int y, SDL_Color col) {
    SDL_Surface* s = TTF_RenderText_Blended(font, text, 0, col);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(ren, s);
    SDL_FRect dst = { (float)x, (float)y, (float)s->w, (float)s->h };
    SDL_DestroySurface(s);
    SDL_RenderTexture(ren, t, nullptr, &dst);
    SDL_DestroyTexture(t);
}