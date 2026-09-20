# Port do MaxLinear PEF32001/PEF32002 (DXS101/DXS102)

## Material confirmado

O pacote DXS TAPI SP 2.0 contém o `drv_tapi_dxs-1.0.0.0`, firmware 2.1.9,
coeficientes 0.0.9 e a biblioteca de testes de linha. O fonte do driver é
dual-license GPL-2.0/BSD-2-Clause e foi preservado em
`vendor/maxlinear/drv_tapi_dxs-1.0.0.0`.

O DXS usa SPI mode 3. A referência atual permite até 8 MHz; a documentação
antiga do Voice System Package 4.44 limitava a integração a 2 MHz. O primeiro
bring-up deve usar 2 MHz e só aumentar a frequência depois de validar leitura
de registradores, firmware e mailbox sem erros.

O protocolo de registradores usa cabeçalho de dois bytes. Escrita usa
`0x7e, offset`; leitura usa `0xbe, offset`. O bit 0 do segundo byte habilita
auto-incremento para transferências com mais de uma palavra, exceto no
registrador de mailbox `DXS_HOST_DATA`. Registradores comuns são palavras de
16 bits; a mailbox transporta palavras de 32 bits. O chip-select deve permanecer
ativo durante cabeçalho e payload.

## Topologia elétrica

O BBD define coeficientes de linha, ringing, alimentação DC e a topologia do
conversor. O arquivo histórico `firmware/dxs/DXS_BBD.bin` coincide exatamente
com `dcdc_CIBB12/R600.bin` do pacote oficial:

| Campo      | Valor confirmado                                                   |
| ---------- | ------------------------------------------------------------------ |
| DC/DC      | CIBB12                                                             |
| Impedância | 600 ohms                                                           |
| Tamanho    | 498 bytes                                                          |
| SHA-256    | `e0c84932582aa8a2931c59d910896f9b1184d189f59af1574ef8fc586354261d` |

Essa correspondência é válida para o blob, não prova que toda placa com
PEF32001/PEF32002 usa CIBB12. Antes de habilitar line feed ou ringing em outra
placa, a topologia precisa ser confirmada pelo esquema, BOM ou pelo firmware
stock da própria placa.

## Arquitetura escolhida

O código MaxLinear inclui TAPI, parser BBD, download de firmware, mailbox,
ring, hook, PCM e testes de linha. Reescrever essas máquinas de estado em um
driver pequeno descartaria validações importantes. A primeira integração mantém
o `drv_tapi_dxs` como módulo independente. Uma ponte posterior usa a Kernel API
TAPI (`ifx_tapi_kopen`, `ifx_tapi_kioctl`, `ifx_tapi_kclose`) para registrar as
linhas no `en75xx_voice`, sem duplicar o controle elétrico.

O módulo não deve ser carregado automaticamente até existirem:

1. seleção explícita da topologia DC/DC por placa;
2. firmware e BBD correspondentes à revisão e ao circuito;
3. nó SPI mode 3 com reset correto;
4. configuração PCM com slots iguais aos programados no EN75xx;
5. ponte TAPI para `/dev/en75xx-fxsN` ou suporte equivalente no userspace.

## Device Tree mínimo

```dts
dxs@0 {
	compatible = "maxlinear,pef32001"; /* ou maxlinear,pef32002 */
	reg = <0>;
	spi-max-frequency = <2000000>;
	spi-cpol;
	spi-cpha;
	reset-gpios = <&gpio N GPIO_ACTIVE_HIGH>;
	reset-interval-ms = <10>;
};
```

O driver original usa os nomes legados `intel,reset-gpios` e
`intel,reset-interval`. A adaptação para DT moderno deve aceitar
`reset-gpios` e `reset-interval-ms`, mantendo os nomes antigos como fallback.
