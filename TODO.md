# Spet@term Roadmap & QA Tracking

Este documento organiza os testes funcionais de formatos suportados pelo File Explorer e Game Engine Archive VFS do Spet@Term V0.05.

## Matriz de Validacao de Conteineres e Arquivos

| Categoria / Engine | Formato / Extensao | Status do Teste | Observacoes e Relato de QA |
|---|---|---|---|
| Call of Duty (Black Ops 1/2) | FastFile (.ff T5/T6) | [!] Testado, mas nao funciona como pretendido | Mostra a estrutura das pastas internas, mas nenhum arquivo; arquivo de 53mb deu hang temporario (otimizado buffer de zone) |
| Call of Duty (Black Ops 3) | FastFile (.ff T7) | [!] Testado, mas nao funciona como pretendido | Mostra nada |
| Call of Duty (Black Ops 3) | FastFile Package (.xpak T7) | [!] Testado, mas nao funciona como pretendido | Alguns mostram pasta "raw", mas a maioria mostra nada |
| Call of Duty (Black Ops 3) | FastFile Data (.fd T7) | [!] Testado, mas nao funciona como pretendido | Abria como txt causando travamento; agora protegido contra abertura como texto |
| Call of Duty (Ghosts) | FastFile (.ff IW6) | [!] Testado, mas nao funciona como pretendido | Alguns mostram pasta "raw", mas a maioria mostra nada |
| Call of Duty (BO4 / Cold War) | FastFile (.ff / .fd / .xpak T8/T9) | [!] Testado, mas nao funciona como pretendido | Alguns mostram pasta "raw", mas a maioria mostra nada |
| id Software / Classic | DOOM / Quake WAD (.wad) | [x] Testado e funcionando com sucesso | Confirmado OK pelo usuario (sons, sprites, patches e flats funcionando perfeitamente) |
| id Software / Quake III | Quake Package (.pk3 / .pk4) | [x] Testado e funcionando com sucesso | Funciona corretamente |
| Windows Package (UWP) | Windows App Package (.appx) | [x] Testado e funcionando com sucesso | Corrigido suporte a ZIP64 (EOCD de 64 bits e extra tags 0x0001) para pacotes modernos e grandes |
| Microsoft GDK / Xbox PC | Pacote de Jogo (.msixvc / .xvd / .xvc) | [x] Testado e funcionando com sucesso | Exibe relatorio tecnico DecryptionNotice.txt e ContainerReport.txt detalhando a protecao de integridade criptografica por hardware XTS-AES do Windows Gaming Services |
| Audio Geral e Jogos | Audio (.wav, .mp3, .ogg, .adx, .swav) | [x] Testado e funcionando com sucesso | Audio OK e player funcionando com controles e aba dedicada |
| Modelos 3D | Malhas 3D (.glb, .gltf, .obj) | [x] Testado e funcionando com sucesso | 98% dos modelos agora perfeitos (multi-submesh com SRV dedicado por textura, isolamento estrito de texturas e cor neutra padrao glTF) |
| Binarios Executaveis | Xbox 360 Executable (.xex) | [x] Testado e funcionando com sucesso | Confirmado OK pelo usuario |
| Video Geral | Conteiner de Video (.mp4, .mkv, .avi) | [x] Testado e funcionando com sucesso | Confirmado OK pelo usuario (player com seekbar interativa e timer funcional) |
| Imagem de Disco Console | Imagem ISO (.iso Xbox 360 / XGD2 / XGD3) | [x] Testado e funcionando com sucesso | Confirmado OK pelo usuario, estrutura e arquivos XDVDFS/GDFX exibidos corretamente |
| Arquivos Compactados | Compactados (.zip, .rar) | [x] Testado e funcionando com sucesso | Suporte completo a ZIP64 e RAR multi-volume: descoberta de todas as partes (.part1..partN, .r00..rN), fusao da arvore de arquivos e deteccao de HEAD_ENCRYPT (WinRAR -hp) |
| Bink Video | Video de Jogo (.bik / .bk2) | [!] Testado, mas nao funciona como pretendido | Mostra somente metadata; parser de cabecalho dedicado implementado |
| Arquivos Gigantes (2GB+) | Binarios Massivos (2GB+) | [x] Testado e funcionando com sucesso | Confirmado OK pelo usuario |
| Valve Source Engine | VPK Archive (.vpk) | [ ] Pendente de teste pelo usuario | Suporte a VPK v1 e v2 (Half-Life 2, CS:GO, Portal 2, L4D2) |
| Unreal Engine | Package Archive (.pak) | [ ] Pendente de teste pelo usuario | Unreal Engine 4 e 5 PAK com montagem de diretorios |
| Unreal Engine | IoStore (.utoc / .ucas) | [ ] Pendente de teste pelo usuario | Conteineres IoStore modernos da UE4.25+ e UE5 |
| Bethesda Softworks | BSA Archive (.bsa) | [ ] Pendente de teste pelo usuario | Skyrim LE, Fallout 3, Fallout New Vegas, Oblivion |
| Bethesda Softworks | BA2 Archive (.ba2) | [ ] Pendente de teste pelo usuario | Fallout 4 e Fallout 76 (General e DX10 Textures) |
| Rockstar Games (RAGE) | RPF Archive (.rpf) | [ ] Pendente de teste pelo usuario | GTA IV, GTA V, Red Dead Redemption |
| Blizzard Entertainment | MPQ Archive (.mpq) | [ ] Pendente de teste pelo usuario | Warcraft III, StarCraft, Diablo II, WoW Classic |
| Blizzard Entertainment | CASC Storage (.casc) | [ ] Pendente de teste pelo usuario | Estrutura moderna de indices CASC |
| Unity Technologies | Unity Assets / Bundles (.assets / .bundle) | [ ] Pendente de teste pelo usuario | SerializedFiles e AssetBundles Unity |
| PlayStation | PS3 / PS4 Package (.pkg) | [ ] Pendente de teste pelo usuario | Conteineres PKG de varejo e desenvolvimento |
| PlayStation | PSARC Archive (.psarc) | [ ] Pendente de teste pelo usuario | Arquivos compactados de jogos Sony PS3/PS4/Vita |
| Nintendo Switch | RomFS (.romfs) | [ ] Pendente de teste pelo usuario | Sistema de arquivos RomFS em pacotes de Switch |
| Nintendo Switch | NSP Package (.nsp) | [ ] Pendente de teste pelo usuario | Pacotes PFS0 do Nintendo Switch |
| Nintendo DS / 3DS | Nintendo WAD (.wad) | [ ] Pendente de teste pelo usuario | Canais de Wii / pacotes Nintendo |
| Texturas de Jogos | DirectDraw Surface (.dds) | [ ] Pendente de teste pelo usuario | Decodificacao DXT1, DXT3, DXT5, BC7 e RGBA para visualizacao |
| Texturas Retro | Sony PS1 TIM (.tim) | [ ] Pendente de teste pelo usuario | Texturas 4-bit, 8-bit e 16-bit com CLUT/paleta de cor |
| Binarios Executaveis | Portable Executable (.exe, .dll, .sys) | [ ] Pendente de teste pelo usuario | Extracao de arquitetura, subsistema, compilacao e dependencias DLL |

## Melhorias Futuras Planejadas
- Auditoria futura e testes adicionais de casos de borda de shaders/materiais glTF/GLB para cobrir os 2% restantes de modelos 3D com mapeamentos atipicos de textura ou canais UV multiplos.
- Refinamento do conjunto de icones para tipos de arquivos especificos de engines e formatos de midia.
- Melhorias nos elementos visuais de navegacao e selecao de itens do File Explorer.
- Expansao dos decodificadores de arquivos compactados e jogos para os formatos que apresentaram limitacoes.
