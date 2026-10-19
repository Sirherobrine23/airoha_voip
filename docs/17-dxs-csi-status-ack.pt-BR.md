# Comparação adicional: spi.ko.c e slic3.ko.c

## Escopo e evidência

Análise estática dos anexos; nenhum teste físico nesta etapa. O patch é uma
correção de sequência em relação ao decompilado OEM, ainda experimental no
hardware. Não há confirmação de que resolva a falha de calibração.

| Arquivo | SHA-256 | Resultado |
|---|---|---|
| spi.ko.c | 718c1c5067fa4c7b5cc5f9b8a591034433df1ff77dba2bd55eed208db7729ade | Idêntico ao mitra-spi.ko.c comparado anteriormente |
| slic3.ko.c | 8526ff84791b0e284a562d205b69b495be7ba874b70a7b23d94e9075e7ab0bb0 | Driver VoicePath Zarlink/Microchip; não é slic3_intel |

Referências de origem são nomes de funções no anexo, não símbolos presumidos.
O ELF correspondente a spi.ko não está disponível neste lote para conferir
as instruções e relocações. Protótipos incompletos, variáveis extraout_* e
chamadas aparentes a _GET_Log_moudle_switch são artefatos possíveis do Ghidra;
não constituem código pronto para recompilar.

## Transporte compartilhado

Em spi.ko.c, spi_wraper_cfgtime_cmd() programa CFG como
`(old & 0xe0ffffef) | 0x0f000010`. CSI_bytes_read()/CSI_bytes_write() chamam
essa preparação uma vez, antes das duas chamadas de envio do cabeçalho e do
laço de dados. O controlador atual já prepara CFG por mensagem SPI, evitando
reconfigurá-lo entre o cabeçalho TX e os dados RX.

No wrapper de base física 0x1fbd1000, com passo 0x2000 por instância:

| Offset | Papel observado | Ação OEM |
|---|---|---|
| +0x00 | CFG | Preparação da transação CSI |
| +0x04 | TXDATA | Envio de byte |
| +0x08 | STATUS | Bit 0 inicia leitura; espera bit 1 TX ou bit 2 RX |
| +0x0c | RXDATA | Leitura do byte recebido |

spi_wraper_write_cmd()/spi_wraper_write_data() esperam `STATUS & 2`,
**releem STATUS** e escrevem `status | 2`. spi_wraper_read_data() inicia com
`status | 1`, espera `STATUS & 4`, lê RXDATA, **relê STATUS** e escreve
`status | 4`.

O port antes escrevia apenas 2 e 4 na confirmação. Este patch reproduz a
releitura e o OR nas confirmações TX/RX apenas para
`airoha,en7523-csi-spi`. Os demais compatibles mantêm sua sequência anterior.
Os limites de espera permanecem; não se copiam os loops infinitos do OEM.

Essa mudança pode importar para bits de controle presentes no mesmo registro,
mas a semântica completa de STATUS não foi estabelecida por documentação.
Um read/OR/write também reescreve outros bits de status que estejam ativos;
reproduzir o OEM não demonstra que essa seja a causa do defeito.

## Conteúdo de slic3.ko.c

init_module() seleciona SPI=0, ZSI=1, ISI=2 e CSI=3. Essa enumeração comum
não significa que todos os modelos listados funcionem em todas as interfaces.
get_slic_type() reconhece zl88601, zl88801, le89116, le89156, le89316,
le9641, le9651, le9652, le9622, le9642 e le9662.
slic_init_device() utiliza VpMakeDeviceObject(), VpMakeLineObject(),
VpInitDevice() e VpSetOption(). As rotinas SLICMGR ajudam a recuperar a
integração de hook, ring e estado de linha para um futuro backend Zarlink.

Não foram encontrados identificadores DXS, PEF ou Intel nesse arquivo.
Seus perfis VoicePath e sua seleção de alimentação não servem como BBD ou
configuração DC/DC do PEF32001. Não há nova sequência de calibração DXS aqui.

## Relação com a falha atual

No último log, os contadores de polling e leitura de INT1 avançam durante a
espera, mas a outbox não entrega novos eventos. CFG/IEN1/IEN2/LEN/BINF seguem
legíveis. Isso afasta a hipótese de callback parado naquele intervalo,
sem provar que o comando ALM foi recebido/processado pelo firmware.
`cached_fw` não é uma consulta ao firmware em execução. Não houve novo
BOOT_FIN nesse último ensaio, diferentemente dos logs anteriores.

O erro 0x706d continua sendo a espera de calibração expirada, encapsulada
como 0x7055 no download BBD. Não se deve forçar sucesso, desativar calibração
ou alterar o perfil DC/DC para contornar esse erro.

## Aplicação e teste

Patch incremental sobre a árvore com o controlador ISI/CSI e as correções
anteriores. Na raiz do repositório:

```sh
git apply --check /caminho/airoha-voip-en7523-csi-status-ack.patch
git apply /caminho/airoha-voip-en7523-csi-status-ack.patch
python3 -m unittest discover -s tests -p test_csi_status_ack.py -v
```

Recompile o pacote que fornece en75xx_isi_spi para o mesmo kernel/target,
instale-o e reinicie para garantir que o controlador novo esteja carregado.
Este patch modifica o controlador, não somente en75xx-slic-dxs.ko.
Mantenha o mesmo firmware, BBD e DTS durante a comparação.

Após o boot, comece pelo teste de firmware:

```sh
modprobe drv_tapi_dxs
echo 1 > /sys/module/drv_tapi_dxs/parameters/event_trace
# Se o adaptador estiver carregado, remova-o antes do insmod abaixo.
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=firmware
```

Se passar, remova o adaptador e repita o ensaio de voz já utilizado:

```sh
rmmod en75xx_slic_dxs &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice
dmesg | tail -n 200
```

O critério é calibração concluída e registro FXS, não apenas ausência de
Unknown symbol ou módulo presente em lsmod. Se persistir o timeout,
compare o novo trecho de eventos e os snapshots do observador existente.
Não há motivo demonstrado aqui para aumentar o timeout.

Validação local: funções C reais extraídas do controlador compiladas com
-Wall -Wextra -Werror em um modelo MMIO; conferidos TX/RX, releitura após
RXDATA, timeout sem confirmação de sucesso e escopo do quirk. Isso não é
compilação do módulo para OpenWrt nem validação elétrica ou funcional na placa.
