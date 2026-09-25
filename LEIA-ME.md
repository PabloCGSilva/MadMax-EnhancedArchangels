# Mad Max – Enhanced Archangels (v0.9.1-beta2)

Quatro Archangels novos, um bônus para cada Archangel e correções nas telas
que nunca esperaram mais de dezesseis.

**Isto é um beta.** Faça backup dos seus saves antes de testar.

## Novidades do beta2

* **Correção para Steam.** Na Steam a versão anterior não encontrava nenhuma das funções do jogo: o executável da Steam mantém o código criptografado até o jogo começar (o DRM da Steam), e o mod procurava cedo demais. Agora ele espera o jogo começar antes de procurar. Testado simulando esse início na versão da GOG; quem joga na Steam, por favor mande o `scripts\EnhancedArchangels.log` se algo der errado.

## Os Archangels novos

| Archangel | Carroceria | Pintura | Montagem | Bônus |
|---|---|---|---|---|
| **Crusader of Torments** | Ripper | Black Tar | aríete e blindagem no máximo, espinhos, grinders | Aríete |
| **Skull Collector** | Ripper | Primer (vermelho) | Ultimate Big Chief V8, escapamento President, espinhos | Motor |
| **Noble Vagrant** | Ripper | Gold Tar | Slick Rubs, suspensão Big Whammy, motor pequeno | Tração |
| **Penitent Stalker** | Wild Hunt | Camuflagem | Big Chief V8, suspensão Big Mama, as peças menos usadas do jogo | Tração |

Eles ficam no fim da lista de Archangels, com nome, descrição e imagem
próprios, na garagem, nos Colecionáveis e na tela *Choose Vehicle* das
fortalezas. Como os originais, cada um está ligado a uma Death Run (Tricky
Pass, Mortal Bite, Heat Haze e Even Rip) e é montado com peças que você tem.

A carroceria Ripper vem do conteúdo *The Ripper* do jogo; Crusader of
Torments, Skull Collector e Noble Vagrant precisam dele.

## Um bônus para cada Archangel

Cada um dos 20 Archangels dá o bônus de **um hood ornament**, escolhido
automaticamente pela montagem:

| Grupo | Efeito | Archangels |
|---|---|---|
| **Motor** | +2,5% de velocidade máxima, +1% de aceleração | Speed Demon, Sanguine Guardian, Argent Cavalier, Cardinal Grinder, Speed Freak, Skull Collector |
| **Tração** | mais aderência dos pneus | Aurelian the Ready, Rule of War, Lord Gravel, Noble Vagrant, Penitent Stalker |
| **Aríete** | batidas mais fortes | Kill Box, Radiant Shadow, Jugger of Virtue, Crusader of Torments |
| **Blindagem** | +20% de resistência a fogo, derruba invasores com mais facilidade | Soul Sweeper, Righteous Spike |
| **Armas** | arpão recarrega 10% mais rápido, side burners gastam 20% menos | The Jack, Pinky Finger, Celestial Bones |

* É o efeito do próprio ornamento do jogo, então aparece como **+** no atributo.
* Só vale com o Archangel instalado como ele é. Troque uma peça e ele deixa de
  ser aquele Archangel, então o bônus sai.
* Os dois espaços de ornamento continuam livres. Um ornamento de verdade do
  mesmo grupo soma: **++**, e o terceiro aparece como um único **+ vermelho**.
* Enquanto você navega pela tela de Archangels, o **+** acompanha o Archangel
  que está vendo.

## Correções

* **Mais de 16 Archangels.** A tela da garagem tinha lugar para exatamente
  dezesseis e travava com mais; os Colecionáveis e o *Choose Vehicle* só
  mostravam dezesseis, e escolher o 17º no *Choose Vehicle* deixava a tela
  preta. Tudo corrigido (a garagem agora aceita até 64).
* **Barra de Handling passando da moldura.** Um bug do jogo original: com o
  Handling do seu carro abaixo de zero e o do Archangel visto acima de zero, a
  parte verde era desenhada comprida demais.

## Instalação

1. Copie tudo deste pacote para a pasta do Mad Max (a que tem o
   `AVAMain.exe`), juntando as pastas `scripts` e `dropzone`.
2. `dinput8.dll` é o Ultimate ASI Loader. Se você já tem ele de outro mod,
   mantenha o seu.
3. Abra o jogo. `scripts\EnhancedArchangels.log` mostra o que o mod encontrou.

Para desinstalar, apague `scripts\EnhancedArchangels.asi` e os arquivos que
este pacote colocou em `dropzone`. Um carro montado a partir de um Archangel
novo mantém as peças.

## Requisitos e compatibilidade

* Feito e testado na versão da **GOG**. Steam deve funcionar desde o beta2, mas ainda não foi confirmada: se o mod não
  encontrar o que precisa no executável, ele se desativa e avisa, e o jogo
  continua seguro.
* Conteúdo *The Ripper*, para três dos Archangels novos.
* Ele substitui, pela dropzone, a tabela de Archangels
  (`vehicles/archetypes.xlsc`), as telas da garagem e dos Colecionáveis
  (`gui/npc_menu_upgrades2.guixc`, `gui/ingame_collectibles2.guixc`) e a lista
  de imagens da interface (`gui/texturelist.guistreamertexturelistc`). Outro
  mod que mude os mesmos arquivos não funciona junto com este.
* Funciona junto com Enhanced Convoys e Wasteland Storms.

## Limitações conhecidas

* As imagens dos Archangels novos são recolorações de uma imagem existente (a
  carroceria Wild Hunt), não imagens dos próprios carros.

## Agradecimentos

* **Rick Gibbed** pelas ferramentas Gibbed do Mad Max, e **gigaHours** pelo
  fork com o montador e desmontador de scripts XVM usado neste mod.
* **Tsuda Kageyu** pelo MinHook, **ThirteenAG** pelo Ultimate ASI Loader.
* **Avalanche Studios** pelo Mad Max.
