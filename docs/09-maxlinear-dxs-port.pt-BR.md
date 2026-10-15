# MaxLinear PEF32001/PEF32002: port DXS/TAPI

## Estado deste port

Base: `main` a9a6ede do `airoha_voip`. Foram implementados o transporte CSI,
a adaptação do backend MaxLinear e a ponte `en75xx-slic-dxs` para a ABI
`/dev/en75xx-fxsN`. Ainda não houve teste deste port em hardware DXS.
A inicialização dos exemplos DTS é **identify**, que testa acesso a registradores
e reinicia o controlador DXS, sem carregar firmware/BBD nem ativar linefeed.
Isso altera clocks, mux e registradores; não é uma operação somente de leitura.

| Placa escolhida | SoC | SLIC | Canais previstos | DTS |
|---|---|---|---|---|
| Nokia G-240G-E | EN751221 | um PEF32002 | 2 | `dts/nokia-g-240g-e-dxs.dtsi` |
| MitraStar GPT-2742GX4X5v6 | EN7523 | PEF32001 | 1 no exemplo | `dts/mitrastar-gpt2742gx4x5v6-dxs.dtsi` |

O loader MitraStar admite descoberta de outro SLIC; a população real da placa
precisa ser conferida antes de adicionar um segundo dispositivo. Askey RTF8225VW
e Nokia XS-2426G-A não foram habilitados por este patch.

## Correção: Nokia também usa CSI

A identificação anterior como ZSI veio do argumento `type=ZSI` passado a
`slic3_sep.ko`. Ela não representa a seleção final do driver DXS: na decompilação
específica do Nokia, `dxs_init_spi()` força `slicInterfType=3` e chama
`SPI_cfg()`/`SPI_Reset(3)`. No `spi.ko` correspondente, 3 seleciona **CSI**.
O modo lógico e o enquadramento são diferentes de ZSI, embora ambos selecionem
5 no campo de interface de SYS SCU + 0x94.

O compatible correto é `econet,en751221-csi-spi`. O antigo
`econet,en751221-zsi-spi` foi removido; o driver ZSI para Zarlink permanece.
Para EN7523, usa-se `airoha,en7523-csi-spi`.

## Protocolo recuperado

DXS utiliza cabeçalho `7e offset` para escrita e `be offset` para leitura,
com palavras em ordem de bytes big endian. O mailbox é o registro 0x14.
O framing CSI abaixo aparece tanto no Nokia quanto no MitraStar:

| Operação | Framing CSI implementado |
|---|---|
| Escrita de um registro de 16 bits | dois bytes de cabeçalho + palavra duplicada; exemplo `7e 06 12 34 12 34` |
| Leitura | dois bytes de cabeçalho, depois leitura de 10 bytes; descarta o primeiro byte e aproveita até 4 bytes |
| Mailbox | fragmentos de até 4 bytes, sem duplicar a palavra e sem auto-incremento |
| Configuração do wrapper | `(CFG & 0xe0ffffef) \| 0x0f000010`; intervalo de 10 µs entre bytes |

As peculiaridades CSI ficam restritas aos dois compatibles CSI. O caminho SPI
convencional conserva o auto-incremento e o enquadramento original.
No EN751221, os gates recuperados de `pcm1.ko` são chip SCU + 0xd8 bit 0 e
+ 0x148 bits 2/3. O pinctrl controla o mux; não foi inventado um divisor absoluto.
No EN7523 foi mantida a sequência existente de clock/APLL do wrapper;
clocks e timing CSI ainda precisam de medição na placa.

## Firmware e BBD por placa

| Imagem | Evidência | Tamanho | SHA-256 |
|---|---|---|---|
| Nokia `DXS_FW.bin` | arquivo stock; cabeçalho 2.1.0 | 16984 | `708f929decc967544c03295dbc7601ec6eda24c7197166d1d757afcabf3dd882` |
| MitraStar `DXS_FW_mitrastar.bin` | embutido nos três módulos Intel recuperados; cabeçalho 2.1.3 | 20168 | `779afa0255e079319ccb264acba714816505c7bcda60c73b967b0c2259dae824` |
| `DXS_BBD.bin` | stock Nokia, presente nos três módulos MitraStar e idêntico ao perfil oficial `coef_dxs-0.0.9/dcdc_CIBB12/R600.bin` | 498 | `e0c84932582aa8a2931c59d910896f9b1184d189f59af1574ef8fc586354261d` |

No MitraStar, `IntelSlicInit()` passa o endereço `.data + 0x94` e tamanho 0x4ec8
a `DXS_DwldAndStartFW()`; as relocations ARM confirmam o símbolo `.LANCHOR2`.
Nos módulos `slic3_intel.ko` e `slic3_intel_latam_25.ko`, o blob começa no offset
183884; no `slic3_intel_latam_25_BR.ko`, em 185020. O extrator procura o cabeçalho
e exige o hash completo, em vez de confiar somente nesses offsets.

Extraia do módulo OEM disponível localmente **antes** de compilar o pacote:

```sh
python3 tools/extract-dxs-mitrastar.py /caminho/slic3_intel.ko \
    firmware/dxs/DXS_FW_mitrastar.bin
```

O pacote OpenWrt inclui essa imagem quando o arquivo existe. O firmware Nokia
não é usado como fallback silencioso no MitraStar. O perfil CIBB12/R600 é
específico do circuito; não equivale a confirmar sua adequação regulatória
para qualquer país ou outra placa PEF3200x. O parser original valida o BBD e a
topologia compilada. O firmware oficial 2.1.9 recuperado do SDK não substitui
automaticamente as imagens OEM.

## Integração implementada

O backend preserva TAPI, download de firmware, mailbox, parser BBD, controle de
alimentação e ringing da MaxLinear. A ponte usa a Kernel API TAPI3 para as
operações de linha. Firmware e BBD têm entradas específicas para buffers do
kernel: não passam ponteiros de `request_firmware()` por `copy_from_user()`.

A ponte configura PCM linear de 16 bits/8 kHz, hook, linefeed, cadência e
ring trip; consulta eventos a cada 20 ms. Os IDs de fault ficam disponíveis na
ABI como o último evento de falha TAPI observado. IRQ externo do DXS não foi
presumido: este port usa o polling do backend. O IRQ do PCM continua vindo do DTS.
O registro de linhas reutiliza `en75xx_voice`, permitindo o mesmo teste PCM e
`chan_en75xx` após inicialização bem-sucedida.

A numeração TAPI é estável pelo chip select, com override opcional
`airoha,tapi-device`. Corrigidos reset com polaridade lógica do GPIO,
propagação de probe defer, registro único do driver SPI, limpeza de mutexes
após falha e major dinâmico sem truncamento em 8 bits.

## DTS e sequência de teste

Inclua somente o fragmento da placa depois do DTS base do SoC. Eles esperam
os labels `scuclk`, `chip_scu`, `pinctrl`, `pcm0` e os bindings de clock/reset
do kernel. O fragmento importa a definição voice do repositório; não inclua
novamente essa mesma definição no DTS da placa. CSI substitui a alternativa ZSI
no mesmo endereço. Não mantenha outro cliente ISI/SLIC ativo nesse wrapper.

Não foi atribuído um GPIO de reset físico: o `gpio_tool write 254` stock é uma
interface lógica OEM e não prova Linux GPIO 254. Acrescente `reset-gpios` e sua
polaridade somente quando confirmados. Sem essa propriedade, o backend depende
do estado de reset externo/boot; a tentativa de acesso pode falhar.

Selecione `kmod-en75xx-slic-dxs-cibb12` no OpenWrt. Ele inclui `drv_tapi_dxs.ko`
e `en75xx-slic-dxs.ko`, com as dependências PCM/serial e firmware. O DXS não é
autocarregado por este pacote. Para o primeiro teste com o DTS em identify:

```sh
modprobe drv_tapi_dxs
modprobe en75xx-slic-dxs
dmesg | grep -Ei 'dxs|csi|en75xx'
```

O resultado esperado é `DXS access test passed`. **Não** se espera
`/dev/en75xx-fxs*` em identify. Se o acesso falhar, confira reset, mux, clocks
e framing antes de avançar.

Para habilitar voice, o nó `&dxs_fxs` deve mudar `airoha,bringup-mode` para
`"voice"` e declarar `clock-frequency`, `maxlinear,pcm-tx-edge` e
`maxlinear,pcm-rx-edge`. As edges são da perspectiva do DXS: 0=subida,
1=descida. A frequência é o DCL PCM, não `spi-max-frequency`. O backend aceita
1.024/2.048/4.096/8.192 MHz neste adaptador. Use a frequência e as edges
verificadas no hardware; não foram fixadas nos DTS de identificação.
Firmware, BBD e `maxlinear,dcdc-topology = "CIBB12"` já estão explícitos.

Slots TAPI contam bytes; a tabela do SoC conta bits. A ponte programa
**DXS RX = SoC TX / 8** e **DXS TX = SoC RX / 8**. O exemplo MitraStar usa
bits TX/RX 24/32, seguindo os slots RX/TX 3/4 de `IntelSlicInit()`.
O exemplo Nokia usa bits 32/48 para os dois canais em ambas as direções;
essa escolha é consistente internamente, mas ainda depende de validação do
framing PCM e do clock da placa.

Após voice inicializar e registrar as linhas, teste info/hook, depois áudio em
baixo nível e somente então ringing. O acesso CSI bem-sucedido não comprova o
caminho de áudio nem a parte analógica. Nenhuma afirmação de ausência de risco
elétrico é feita por este port ainda não validado.

## Compilação e validação executadas

`tools/build-dxs.sh` compila primeiro o backend e passa seu `Module.symvers`
ao Kbuild da ponte. Exige um kernel preparado e sua toolchain; aceita argumentos
extras do configure. A integração OpenWrt usa a mesma ordem de compilação.

Validação nesta sessão:

- 11 testes host passaram, incluindo funções C reais de acesso DXS com I/O
  simulado: CSI nos dois compatibles, prefixo, padding, mailbox, erro de
  transporte e preservação do SPI convencional.
- O extrator recuperou a mesma imagem dos três módulos MitraStar e rejeitou
  dados truncados/corrompidos.
- Backend e módulos EN75xx compilaram contra Linux 6.18.55, commit
  `21c8344c030504a8e85dfb1c84cf4970e754302a`, configuração de compilação x86_64
  com OF/SPI/GPIOLIB/REGMAP habilitados.
- O kernel não foi compilado por completo: faltava seu `Module.symvers`, então
  modpost foi executado com warnings para exports do kernel. Os exports entre
  backend, ponte e núcleo EN75xx foram conferidos separadamente. Isso não
  equivale a build OpenWrt completo, cross-build ARM/MIPS ou teste de carga.
- Ambos os fragmentos DTS foram compilados com dtc e os DTS base reais desse
  kernel. Warnings já existentes de I²C/switch/watchdog persistem; não foi
  executado dt-schema nem validado o DTS completo de cada placa.

## Fontes consultadas

No `sdk_base` recuperado da biblioteca: `docs/maxlinear/617580_dxs_api_device_driver_pr_rev3.0.pdf`
e `docs/maxlinear/PEF32001VSV13.pdf`; os módulos/decompilações Nokia em
`reverse/en751221/nokia-g_240g_e/` e MitraStar em `reverse/en7523/ghidra/mitrastar/`.
O PDF da API DxsInit/DxsBBD não substitui os headers da API TAPI deste port.
Os trechos Ghidra foram usados como evidência de comportamento e confrontados
com relocations/bytes ELF; não foram tratados como fontes C recompiláveis.
O rootfs MitraStar forneceu os módulos usados nas comparações de firmware/BBD.
