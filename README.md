# JARVIS AI — LILYGO T-Embed CC1101 Plus

Primeira versão (MVP): interface no display, Wi-Fi, página de controle para Android e conversa com IA pela API. A fala é feita pelo navegador Chrome do Android (microfone e alto-falante do telefone); nesta etapa o microfone e o alto-falante internos da placa ainda não são usados.

## Hardware
- LILYGO T-Embed CC1101 / CC1101 Plus (ESP32-S3)
- MicroSD opcional; esta versão não apaga nem formata o cartão.
- Wi-Fi 2,4 GHz.

## Segurança e custos
- É necessária uma chave de API OpenAI, criada na conta do provedor. O uso da API pode ser cobrado separadamente da assinatura ChatGPT.
- Não publique sua chave no GitHub. A versão inicial guarda a chave na memória NVS do ESP32; não compartilhe o aparelho com pessoas não confiáveis.
- Não use a chave de API dentro de páginas públicas.
- Firmware experimental. O protótipo usa `setInsecure()` para HTTPS; não é configuração TLS adequada para produção.

## Como funciona
1. Ligue a placa. Na primeira configuração, conecte o telefone à rede `JARVIS-SETUP`, senha `jarvis123`.
2. O portal Wi-Fi abre para escolher sua rede 2,4 GHz.
3. Depois de conectar, veja o IP mostrado na tela/roteador e abra `http://IP-DA-PLACA` no Chrome do Android.
4. A página permite digitar ou falar em português. O reconhecimento de voz depende do suporte do Chrome e da permissão de microfone.
5. A resposta aparece no celular e no display da placa.

## Chave da API
Na página do JARVIS, cole a chave de API no campo próprio e toque em **Salvar chave**. Ela é gravada na memória NVS do ESP32 e não fica no código-fonte nem no GitHub. Use apenas em uma rede Wi-Fi confiável; qualquer pessoa com acesso local à página pode tentar enviar comandos ao aparelho.

## Compilação
O GitHub Actions compila automaticamente quando o projeto é enviado ao repositório. O artefato gerado será `firmware.bin` (e arquivos auxiliares do bootloader/partições, conforme o build). Para gravar pelo Android, use um aplicativo de gravação ESP32 compatível com ESP32-S3 e os arquivos do build. Não grave antes de confirmar o procedimento e manter uma cópia do firmware original.

## Estado desta versão
- [x] Tela ST7789
- [x] Wi-Fi por portal
- [x] Página web para Android
- [x] Pergunta/resposta via API
- [x] Voz pelo Android
- [ ] Configuração segura da chave pela página local
- [ ] Microfone e alto-falante integrados da placa
- [ ] Histórico, wake word e comandos locais

## Referências
- https://github.com/Xinyuan-LilyGO/T-Embed-CC1101
- https://github.com/cjrazevedo/JARVIS-AI-LILYGO
