# Port do MaxLinear PEF32001/PEF32002 (DXS101/DXS102)

## Material confirmado

O pacote DXS TAPI SP 2.0 contém o `drv_tapi_dxs-1.0.0.0`, firmware 2.1.9,
coeficientes 0.0.9 e a biblioteca de testes de linha. O fonte do driver é
dual-license GPL-2.0/BSD-2-Clause e foi preservado em
`vendor/maxlinear/drv_tapi_dxs-1.0.0.0`.

O DXS usa SPI mode 3 no protocolo lógico. Nas placas EN7523 Askey RTF8225VW
e MitraStar GPT-2742GX4X5v6, porém, esse protocolo passa pelo wrapper CSI
compartilhado com o PCM, e não por um controlador SPI independente. O firmware
OEM detecta explicitamente `Lantiq CSI` e carrega
`slic3_intel.ko type=CSI slic=pef32001 mode=IB`.

O rootfs stock da Nokia G-240G-E mostra uma integração diferente no
EN751221: `slic3_sep.ko type=ZSI fxsNum=2 devNum=1 pcm1ChNum=2`, seguido por
`drv_ifxos.ko`, `drv_tapi.ko` e `drv_dxs.ko major=93`. O `drv_dxs.ko` importa
`SPI_cfg`, `SPI_Reset` e `SPI_bytes_{read,write}`; portanto, "SPI" é a API
lógica do DXS, enquanto o `spi.ko` stock transporta os bytes por ZSI. O
compatible `econet,en751221-zsi-spi` reproduz essa ponte pelo subsistema SPI
moderno, sem tratar incorretamente a placa como CSI.

A referência atual permite até 8 MHz; a documentação
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

Os dois arquivos no repositório são idênticos aos usados no rootfs de produção
da Nokia G-240G-E:

| Arquivo       | SHA-256                                                            |
| ------------- | ------------------------------------------------------------------ |
| `DXS_FW.bin`  | `708f929decc967544c03295dbc7601ec6eda24c7197166d1d757afcabf3dd882` |
| `DXS_BBD.bin` | `e0c84932582aa8a2931c59d910896f9b1184d189f59af1574ef8fc586354261d` |

Isso confirma CIBB12/R600 para essa placa específica, mas não prova que toda placa com
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

## Matriz de placas confirmada

| SoC      | Placa                     | SLIC               | Situação do transporte                                    |
| -------- | ------------------------- | ------------------ | --------------------------------------------------------- |
| EN751221 | Nokia G-240G-E            | 1× PEF32002, 2 FXS | ZSI; stack stock, clock/route, firmware e BBD confirmados |
| EN7580   | Nokia XS-2426G-A          | PEF32002           | CSI; roteamento/clock do SoC ainda pendente               |
| EN7523   | MitraStar GPT-2742GX4X5v6 | PEF32001           | CSI e build OEM CIBB12 confirmados                        |
| EN7523   | Askey RTF8225VW           | PEF32001           | CSI e build OEM CIBB12 confirmados                        |

Nos dois rootfs EN7523, `voip_loader` suporta um ou dois dispositivos e
acrescenta `devNum=2` somente quando a detecção encontra o segundo SLIC.
Portanto, a quantidade de linhas não deve ser inferida apenas pelo PEF32001.

Na G-240G-E, o arquivo de hardware declara duas portas POTS com PEF32002, mas
o carregador confirma `devNum=1` e `pcm1ChNum=2`: um único DXS102 de dois
canais usando dois slots do PCM1.

## Device Tree mínimo para EN751221/ZSI

```dts
zsi0: spi@1fbd1000 {
	compatible = "econet,en751221-zsi-spi";
	reg = <0x1fbd1000 0x1000>;
	reg-names = "zsi";
	airoha,scu = <&scuclk>;
	airoha,chip-scu = <&chip_scu>;
	airoha,legacy-scu-programming;
	resets = <&scuclk EN751221_PCM1_ZSI_ISI_RST>;
	reset-names = "zsi";
	pinctrl-names = "default";
	pinctrl-0 = <&pcm_spi_pins>;
	airoha,zsi-gap-us = <5000>;
	#address-cells = <1>;
	#size-cells = <0>;

	dxs@0 {
		compatible = "maxlinear,pef32002";
		reg = <0>;
		spi-max-frequency = <2000000>;
		spi-cpol;
		spi-cpha;
		reset-gpios = <&gpio N GPIO_ACTIVE_HIGH>;
		reset-interval-ms = <10>;
	};
};
```

O número do GPIO de reset ainda deve vir do DTS ou esquema da placa. A
sequência stock `gpio_tool write 254 1,0,1` parece endereçar um framework
lógico OEM e não basta para colocar GPIO 254 no DT Linux.

## Device Tree mínimo para EN7523/CSI

```dts
csi: spi@1fbd1000 {
	compatible = "airoha,en7523-csi-spi";
	reg = <0x1fbd1000 0x100>;
	/* clocks, resets e phandles SCU iguais ao nó ISI do SoC */

	dxs@0 {
		compatible = "maxlinear,pef32001";
		reg = <0>;
		spi-max-frequency = <2000000>;
		spi-cpol;
		spi-cpha;
		reset-gpios = <&gpio N GPIO_ACTIVE_HIGH>;
		reset-interval-ms = <10>;
	};
};
```

Os GPIOs e a polaridade de reset continuam pendentes de confirmação por
placa. O transporte CSI não autoriza por si só o download do BBD nem a
ativação de linefeed/ringing.

O driver original usa os nomes legados `intel,reset-gpios` e
`intel,reset-interval`. A adaptação para DT moderno deve aceitar
`reset-gpios` e `reset-interval-ms`, mantendo os nomes antigos como fallback.
