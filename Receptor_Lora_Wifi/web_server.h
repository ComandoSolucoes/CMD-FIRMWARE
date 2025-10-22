#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// Sobe o servidor e registra rotas mínimas (chamar ao subir o AP)
void iniciarServidorWeb();

// Handler da rota raiz "/"
void paginaInicial();
void paginaPainel();

// (opcional, mas recomendado ter)
void processarServidorWeb();  // chame no loop() para server.handleClient()
void pararServidorWeb();      // para o servidor HTTP

#endif // WEB_SERVER_H
