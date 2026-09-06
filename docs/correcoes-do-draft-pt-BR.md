# Correções do draft EN75xx FXS/VoIP

## Escopo e nível de confiança

Esta revisão cruza o draft com os módulos vendor decompilados e com a
árvore Linux usada como alvo. Ela corrige incompatibilidades objetivas,
mas não transforma o projeto em um driver validado em hardware. Toda
informação abaixo é classificada como:

- **confirmada no código atual**: pinctrl, IDs de reset e clock SLIC;
- **confirmada nos módulos vendor**: mapa PCM, stride e campos dos
  descritores, ring count e codificação do endereço DMA;
- **a validar na placa**: IRQ PCM, polaridades externas, clock/routing
  ZSI fora do EN751221 e estabilidade elétrica do SLIC.

## O que foi corrigido

| Área               | Problema do draft                                                                   | Correção                                                                                                |
| ------------------ | ----------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- |
| DMA gen2           | colocava `ch_valid` também no status                                                | EN7523 usa status com `OWN + sample_size`; a máscara fica apenas no word `+0x04`                        |
| EN7523             | não programava `CHAN_ENABLE`                                                        | grava `0xac` com a máscara de canais, limitada a `0x0f`                                                 |
| Descritores        | máscara ativa mudava a geometria durante a chamada                                  | descritores usam uma máscara DMA configurada e estável                                                  |
| Linhas simultâneas | abrir/fechar a segunda linha reiniciava o PCM                                       | o engine inicia na primeira abertura e para na última; as demais só ativam/desativam FIFO               |
| IFACE_CTRL         | escrevia uma vez                                                                    | aplica a borda `CFG_VALID`: clear e depois set do bit 26                                                |
| Interrupções       | máscara ignorava erros 9/10 e era habilitada sem IRQ                                | usa bits 2–10; em polling deixa IMR em zero e limpa ISR observado                                       |
| Endianness         | convertia PCM16 com casts nativos                                                   | a UAPI é sempre S16 little-endian; usa acessos LE desalinhados e swap simétrico TX/RX quando necessário |
| G.711              | os encoders usavam limites de segmento reduzidos e o decoder A-law invertia o sinal | usa a segmentação de 16 bits das implementações de referência do kernel e preserva corretamente o sinal |
| Endereço DMA       | truncava silenciosamente `dma_addr_t`                                               | configura a máscara DMA correta e rejeita rings/buffers fora da janela codificável                      |
| SCU/reset          | PCM gravava diretamente `0x834`                                                     | usa `reset_control_reset()` e IDs do provider atual                                                     |
| Pinmux             | PCM gravava offsets EN751221 em todos os SoCs                                       | pinctrl é o único caminho normal; nenhuma escrita SCU no driver PCM                                     |
| ZSI                | bit 25 era chamado de reset SPI                                                     | o nome atual é `SFC2_PCM_RST`; ZSI usa seu reset específico                                             |
| ZSI                | offsets de clock EN751221 eram aplicados globalmente                                | sequência crua só existe com opt-in e é rejeitada fora do EN751221                                      |
| Robustez ZSI       | erros de regmap/populate eram ignorados; lookup tinha race                          | erros são propagados, lista é revertida no erro e a referência do device é obtida sob lock              |
| Timeslots Le9642   | slots duplicados ou inválidos viravam linha muda                                    | probe rejeita slot menor que 4, fora do range ou duplicado                                              |

## Layout PCM mantido pelo driver

| Item                 | EN751221 / EN7528                  | EN7523                        |
| -------------------- | ---------------------------------- | ----------------------------- |
| geração              | gen1                               | gen2                          |
| descritor            | `0x24` bytes                       | `0x0c` bytes                  |
| descritores por ring | 15                                 | 15                            |
| status               | `OWN`, `ch_valid[23:16]`, amostras | `OWN`, amostras               |
| word `+0x04`         | buffer do canal 0                  | máscara `ch_valid`            |
| buffers              | 8 endereços por descritor          | 1 endereço por descritor      |
| `RING_CFG`           | `0x9f`                             | `0x3f`                        |
| endereço do ring     | `phys & 0x1fffffff`                | `(phys & 0x3fffffff)          | 0x80000000` |
| enable adicional     | não observado                      | `0xac`, máscara máxima `0x0f` |

O frame continua sendo 80 amostras, ou 160 bytes para PCM16. O slab de
um descritor mantém stride de oito canais mesmo no EN7523, cuja máscara
de hardware observada habilita quatro canais.

## Reset, pinctrl e clock

O reset provider atual mapeia os blocos de voz; os valores no DTS são
IDs de binding, não bits crus:

| Função | EN751221                    | EN7528                    | EN7523                          |
| ------ | --------------------------- | ------------------------- | ------------------------------- |
| PCM1   | `EN751221_PCM1_RST`         | `EN7528_PCM1_RST`         | `EN7523_PCM1_RST`               |
| PCM2   | `EN751221_PCM2_RST`         | `EN7528_PCM2_RST`         | não há reset separado do engine |
| ZSI1   | `EN751221_PCM1_ZSI_ISI_RST` | `EN7528_PCM1_ZSI_ISI_RST` | `EN7523_PCM1_ZSI_ISI_RST`       |

No EN751221/EN7528, PCM2 corresponde ao bit 4 de `RST_CTRL1`; o antigo
`0x1000` do draft selecionava o bit 12 e estava errado. Pinctrl também é
específico por SoC: `+0x104` no EN751221, `+0x15c/+0x224` no EN7528 e
`+0x214` no EN7523. Por isso offsets não são mais reutilizados pelo PCM.

O EN7523 expõe `EN7523_CLK_SLIC`, consumido pelo nó ZSI. Esse clock não
é evidência de que o engine PCM use o mesmo clock. O provider MIPS ainda
não expõe um clock SLIC; a compatibilidade crua ficou restrita ao
EN751221, onde a sequência foi observada. EN7528 ZSI permanece pendente
de validação do route/clock correto.

## DTS corrigido

- PCM usa `resets` e `reset-names = "pcm"`.
- ZSI usa `resets` e `reset-names = "zsi"`.
- EN7523 ZSI consome `EN7523_CLK_SLIC` com `clock-names = "slic"`.
- Exemplos de duas linhas usam `airoha,dma-channel-mask = <0x05>` para
  os canais 0 e 2, derivados dos bus slots 4 e 6.
- O segundo PCM do EN7523 não inventa um reset que o binding não possui.
- IRQ 27 do EN7523 continua marcado como candidato do vendor, não como
  fato validado no GIC da placa.

## Validação realizada nesta revisão

- Os cinco módulos (`en75xx-pcm`, `en75xx-voice`, `en75xx-zsi`,
  `en75xx-slic-le9642` e `en75xx-slic-si3219x`) compilam e linkam sem
  warning de compilador com `W=1` contra a árvore Linux 6.18 alvo,
  commit `8f909dac6c5aca0a9c936ce0966a7dce56837427`.
- Os cinco exemplos DTS passam pelo preprocessor e pelo `dtc` construído
  pela mesma árvore.
- `proslic-patch2fw.py` passa por `py_compile`, o instalador OpenWrt
  passa por `sh -n` e as duas cópias de `chan_en75xx.c` são idênticas.

Essa compilação verifica APIs e linkedição dos módulos em uma configuração
x86_64 preparada com `modules_prepare`; ela não substitui o cross-build
ARM/MIPS. Sem o `Module.symvers` completo do kernel, o `modpost` reporta
símbolos do próprio kernel como não resolvidos. O canal Asterisk ainda
precisa ser compilado contra os headers da versão exata usada no firmware.

## Pendências antes de uso real

1. Confirmar no osciloscópio BCLK/FSYNC e a taxa do clock SLIC.
2. Validar o IRQ PCM; até lá, usar polling.
3. Confirmar se `pcm_spi_rst` realmente reseta o componente externo ou
   apenas seleciona uma função de hardware.
4. Testar rings TX/RX por pelo menos 30 minutos, acompanhando OWN,
   underrun, overrun e erro AHB.
5. Testar duas linhas simultâneas, abertura/fechamento independente e
   G.711 em MIPS big-endian.
6. Compilar `chan_en75xx` contra a versão exata do Asterisk do firmware
   e testar carga/descarga, DTMF, ring, answer e hangup.

Até esses itens passarem, trate o código como uma base de porte
auditável, não como suporte de produção.
