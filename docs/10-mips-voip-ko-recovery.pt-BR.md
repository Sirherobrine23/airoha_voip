# Recuperação verificável dos módulos VoIP MIPS

## Escopo e material

O SDK preserva módulos não removidos de símbolos para o perfil
`UNION_EN7528_LE_7592_7613_MAP_R2_demo`. Eles são ELF32 MIPS32r2
little-endian, construídos para Linux 3.18.21 SMP. Não há DWARF, mas existem
`.symtab`, `.strtab`, relocations, `.pdr`, `.reginfo`, nomes locais e tamanhos
das funções.

Esta etapa não incorpora os `.ko` proprietários ao projeto. O utilitário
`tools/voip-ko-recovery.py` gera manifests com hashes, ABI, símbolos,
relocations e hashes por função. Isso permite substituir funções por C
recuperado gradualmente e medir a equivalência sem depender do Ghidra como
critério de aceitação.

| Módulo       | Funções | Imports | Exports | Relocations |
| ------------ | ------: | ------: | ------: | ----------: |
| `pcm1.ko`    |      58 |      46 |       6 |       1.696 |
| `pcm2.ko`    |      57 |      48 |       0 |       1.665 |
| `pcmDump.ko` |      37 |      33 |       0 |         596 |
| `spi.ko`     |      68 |      27 |      10 |       2.060 |
| `sys_mod.ko` |      46 |      19 |      38 |         346 |
| `slic3.ko`   |     424 |      23 |       1 |      12.455 |
| `fxs3.ko`    |     701 |      95 |      93 |      18.450 |

## ABI recuperada

`pcm1.ko` exporta `getPcmConfig`, `pcmChipScuQuery`,
`pcmSlicResetRegQuery`, `pcm_reinit_slic`, `pcm_setFreeRunMode` e
`getGponUpStatus`. `spi.ko` importa as duas funções de consulta de SCU/reset,
portanto a configuração do barramento SLIC depende explicitamente do PCM e
não deve manter uma segunda tabela de endereços.

`spi.ko` exporta:

- `SPI_cfg`, `SPI_Reset` e `pSLIC_Reset`;
- `SPI_bytes_read` e `SPI_bytes_write`;
- variantes ZTE das operações;
- `SPI_set_gpio_for_cs`;
- `SPI_check_slic_valid`;
- `SPI_enable_ZSI_CSI_clocktest`.

Os símbolos internos confirmam quatro transportes distintos no mesmo módulo:
SPI, ZSI, ISI e CSI. Existem pares dedicados `ZSI_bytes_*`, `ISI_bytes_*` e
`CSI_bytes_*`; por isso não é correto tratar ISI como um simples alias de ZSI.
O caminho SPI também possui operações separadas para Zarlink, Silicon Labs e
Lantiq/MaxLinear.

O C do Ghidra pode agora ser limitado pelos símbolos e relocations do módulo
original. Nesse cruzamento, o wrapper serial fica em `0x1fbd1000 + id*0x2000`:
TX/RX em `+0x04`, controle/status em `+0x08` e RX em `+0x0c`. O fluxo recuperado
é:

| Transporte | Prefixo               | Leitura                                | intervalo explícito do módulo antigo |
| ---------- | --------------------- | -------------------------------------- | ------------------------------------ |
| ISI        | `control`, `register` | lê os bytes logo após os dois comandos | nenhum; polling usa 2 us             |
| ZSI        | `register`            | envia também `0x06` antes de ler       | 10 us após cada byte                 |
| CSI        | `control`, `register` | lê após os dois comandos               | 10 us após cada byte                 |

O registrador de modo legado em `0x1fb00094` usa o nibble baixo `0xf` para ISI
e `0x5` para ZSI/CSI. Isto confirma que ISI pode ser apresentado como um
`spi_controller` virtual ao driver ProSLIC: uma escrita SPI de três bytes vira
`control, register, data`, e `spi_write_then_read(control, register)` casa com
o fluxo ISI. O tempo de 10 us do módulo antigo não substitui o intervalo maior
medido em hardware para ZSI; cada transporte mantém sua temporização própria.

`sys_mod.ko` é principalmente uma camada de OS: mutex, task, listas e logging.
Ele não deve ser portado literalmente. Apenas as poucas dependências de IRQ e
logging devem ser substituídas pelas APIs atuais do kernel.

## PCM1 versus PCM2

Os dois módulos compartilham 57 funções nomeadas. Dezoito possuem bytes de
função idênticos; 39 diferem, embora a maioria mantenha exatamente o mesmo
tamanho. Isso indica builds do mesmo fonte com constantes, globais e caminhos
de inicialização diferentes, em vez de dois controladores independentes.

`pcm2.ko` importa `pcm_reinit_slic` e `pcm_setFreeRunMode` de `pcm1.ko` e não
exporta uma ABI própria. `getGponUpStatus` só existe no PCM1. A implementação
nova deve continuar usando uma única classe de driver parametrizada por
recursos do SoC/DT; não há justificativa para duplicar o driver inteiro.

## Registradores confirmados pelos headers MIPS

O `pcmdriver.h` do SDK confirma para a geração MIPS:

| Item                         | PCM1         | PCM2         |
| ---------------------------- | ------------ | ------------ |
| base virtual legado          | `0xbfbd0000` | `0xbfbd2000` |
| base física                  | `0x1fbd0000` | `0x1fbd2000` |
| registradores principais     | 17           | 17           |
| descritores TX/RX            | 15 / 15      | 15 / 15      |
| canais/buffers por descritor | 8            | 8            |
| reset legado                 | `0x800`      | `0x10`       |

Também são confirmados `IMR=0x28`, polling TX/RX em `0x2c/0x30`, controle DMA
em `0x40`, `RING_CFG` em `0x3c` e os bits de interrupção já usados pelo driver.

O header contém `PCM_INT=12` e `PCM2=34` quando `CONFIG_MIPS_TC3262` está
ativo. Esses números não devem ser copiados diretamente para o DT: eles podem
ser números Linux legados após o offset/cascade do INTC. Há evidência anterior
para hwirq 11/32 e um boot do EN7528 mostrou `irq=34` já mapeado pelo Linux.
O driver atual deve continuar recebendo o IRQ pelo DT e registrar tanto hwirq
quanto virq durante o bring-up.

## Uso do verificador

```sh
tools/voip-ko-recovery.py inventory out \
    /caminho/pcm1.ko /caminho/pcm2.ko /caminho/spi.ko

tools/voip-ko-recovery.py compare out/reference.json \
    pcm1.ko /caminho/pcm1-reconstruido.ko
```

Uma comparação exata do módulo final requer o kernel 3.18.21, `.config`,
`Module.symvers`, toolchain e `modpost` originais. Antes disso, os hashes por
função permitem validação incremental. Igualdade de bytes não substitui teste
de hardware, especialmente para reset, line feed, ringing e DC/DC do SLIC.

## Próximos alvos

1. recuperar `pcmConfigSetup`, `descInit`, `rxDescSet` e `pcmSend`;
2. recuperar os quatro pares de transporte do `spi.ko`;
3. mapear as funções recuperadas para `en75xx_pcm.c` e `en75xx_zsi.c`;
4. conferir o `en75xx-isi-spi` usado no teste do XC220-G3v contra o framing
   recuperado acima e habilitar Si32192/Si32193 somente nesse controller;
5. usar `pcmDump.ko` somente como referência do tap RX/TX e não como requisito
   em produção.
