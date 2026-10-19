# MitraStar: comparação OEM e trace da calibração DXS

## Evidência nova da pasta Mitra

Origem: https://drive.google.com/drive/u/0/folders/1Kx1MO_VIQWr7DnYbmFSX9uI8CIbnR3yG

Foram lidos os binários lib/modules/spi.ko,
lib/modules/slic3_intel_latam_25_BR.ko e userfs/bin/voip_loader, além de
usr/script/voip_load.sh, lib_voip, lib_modules e userfs/profile.cfg.
A análise é estática: esses executáveis OEM não foram executados.

| Arquivo | SHA-256 |
|---|---|
| spi.ko, 57.852 bytes | 8ed3cf418fffedbbcb7f97ea73d58cbe148d060460338f9fdafb05421f1debed |
| slic3_intel_latam_25_BR.ko, 306.640 bytes | 602b6658e40c0cfec1518aa9c62c3256d5628871216b78beca79016b1fafb19f |

Ambos são ELF ARM little endian, relocáveis, com símbolos. As instruções e
relocações foram conferidas com pyelftools/Capstone.

### Wrapper SPI/CSI

O spi.ko confirma diretamente o que o Ghidra mostrava:

| Função | Evidência em .text, offsets relativos à seção |
|---|---|
| spi_wraper_write_cmd | 0x1858 chama regReadArm; 0x185c OR com 2; 0x1868 transfere a regWriteArm |
| spi_wraper_write_data | 0x18c4 chama regReadArm; 0x18c8 OR com 2; 0x18d4 transfere a regWriteArm |
| spi_wraper_read_data | 0x193c lê RXDATA; 0x1948 relê STATUS; 0x194c OR com 4; 0x1958 escreve STATUS |

Os acessos usam 0xbfbd1004/1008/100c, com passo 0x2000. São os endereços de
alias usados pelo OEM; o recurso físico do port continua em 0x1fbd1000.
A rotina de escrita recebe o byte em r1, embora o protótipo decompilado de
spi_wraper_write_cmd omita esse argumento. As chamadas indiretas estranhas do
Ghidra possuem relocações para arm_delay_ops: não são chamadas reais ao logger.
Isso elimina a ressalva anterior de que a confirmação STATUS estava apoiada
apenas no decompilado. Ainda não demonstra que ela explique a calibração.

No módulo Intel BR, DXS_RegWriteMulti() salta para .text+0x222d4. O caminho
CSI escolhe tamanho de frame 6 e desconta o cabeçalho de 2 bytes, enviando
4 bytes por fragmento; os DWORDs do mailbox são serializados MSB primeiro.
DXS_RegReadMulti() salta para .text+0x21fd0; o caminho CSI pede 10 bytes e
avança o início útil em um byte. Essas características coincidem com o port.

DXS_ALM_Calibration() passa 0x1f4 (500) a IFXOS_EventWait em .text+0x211ac.
O timeout de aproximadamente 500 ms não foi introduzido pelo nosso port.

### BBD, firmware e seleção do SLIC

O arquivo ELF BR contém, byte a byte:

| Objeto | Offset no arquivo ELF | Tamanho | SHA-256 |
|---|---|---|---|
| BBD já recuperado | 0x281b8 | 498 | e0c84932582aa8a2931c59d910896f9b1184d189f59af1574ef8fc586354261d |
| Firmware Mitra já recuperado | 0x2d2bc | 20.168 | 779afa0255e079319ccb264acba714816505c7bcda60c73b967b0c2259dae824 |

Esses são offsets de arquivo, não endereços de memória para devmem.
O ensaio recente usa firmware que responde 2.1.1; a identidade acima é do
firmware OEM Mitra previamente recuperado. O patch não troca nenhum firmware.

O profile.cfg habilita PI_AUTO_DETECT_SLIC. voip_load.sh chama voip_loader e
contém um bloco condicional VOIP_LOADER_SUCCESS=yes com fallback Zarlink
le9642/ZSI. O binário voip_loader contém formatos para PEF32001/CSI/mode=IB e
nomes dos módulos Intel normal, LATAM e BR. Portanto, aquele bloco de script
não prova que esta unidade tenha Zarlink, nem a presença dos três módulos
prova qual variante foi efetivamente carregada. Não se deve copiar o fallback
Zarlink para o DTS do Mitra com PEF32001.

## O que o novo patch observa

O último log passa DEV_START e os blocos BBD, com consulta de versão válida
antes da calibração. Após opmode=6, polling continua, mas não há novos eventos
da outbox; a consulta de versão posterior também expira. Os registradores
HOST ainda respondem. Isso não determina se o comando foi aceito pelo DXS.

O patch é diagnóstico, incremental sobre as correções anteriores, incluindo
airoha-voip-en7523-csi-status-ack.patch. Não muda firmware, BBD, DTS, clocks,
timeouts, modo de linha ou critérios de sucesso da calibração.

1. drv_tapi_dxs ganha mailbox_trace, desligado por padrão. A solicitação exata
   header=0x01000704/payload=0x00060000 arma uma janela de até 32 fragmentos de
   DATA/LEN/CFG. Não há dump de INT1 a cada polling nem dump de firmware.
2. As escritas guardam os bytes reais preparados para os dois primeiros
   fragmentos e só os imprimem ao terminar a escrita lógica. O comando de
   calibração tem exatamente dois. Leituras registram os bytes brutos antes
   da remoção do byte inicial CSI; buffers de RX com erro não são impressos.
3. en75xx_isi_spi ganha csi_trace, desligado por padrão. Para o par de frames
   abaixo, guarda a conclusão do cabeçalho e só imprime após o payload. Mostra
   done, ret, último STATUS observado pelo polling TX e último valor de ACK.
   Não há leitura MMIO adicional para produzir esse trace.
4. A mensagem de probe identifica ACK read/OR/write ou mask-only, permitindo
   distinguir o controlador novo de um módulo antigo ainda carregado.

Frames esperados:

```text
7e 14 01 00 07 04   cabeçalho do comando
7e 14 00 06 00 00   payload: calibração
```

O trace do controlador casa esses frames completos. Se o backend mostrar bytes
diferentes, as linhas específicas CSI calibration podem não aparecer; isso
não demonstra que o controlador não tenha sido chamado.

Um done=6/6 com ret=0 significa que o wrapper sinalizou conclusão para os seis
bytes. Não prova que o DXS recebeu, interpretou ou executou o comando. Para
verificar os sinais físicos, ainda é necessária uma captura adequada ao CSI.
Os parâmetros e o filtro deste patch são destinados ao canal 0 do Mitra; não
constituem um trace genérico de todos os canais/SLICs.

## Aplicação

Na raiz do repositório, após os patches anteriores:

```sh
git apply --check /caminho/airoha-voip-dxs-mitra-mailbox-trace.patch
git apply /caminho/airoha-voip-dxs-mitra-mailbox-trace.patch
python3 -m unittest discover -s tests -p 'test_*trace.py' -v
python3 -m unittest discover -s tests -p test_csi_status_ack.py -v
```

Recompile e instale os pacotes que fornecem **drv_tapi_dxs.ko e
en75xx_isi_spi.ko**, para o kernel exato do device. Apenas substituir o
adaptador en75xx-slic-dxs.ko não instala o diagnóstico. Reinicie depois de
instalar para garantir que o controlador em uso é o novo.

## Ensaio na placa

Mantenha o mesmo DTS, firmware e BBD do último teste para isolar a alteração.
Após o boot:

```sh
modprobe drv_tapi_dxs
cat /sys/module/en75xx_isi_spi/parameters/csi_trace
cat /sys/module/drv_tapi_dxs/parameters/mailbox_trace

echo 1 > /sys/module/en75xx_isi_spi/parameters/csi_trace
echo 1 > /sys/module/drv_tapi_dxs/parameters/mailbox_trace
echo 1 > /sys/module/drv_tapi_dxs/parameters/event_trace

# Se o adaptador estiver carregado, remova-o antes do insmod.
# Na primeira carga, a mensagem de rmmod dizendo que não está carregado é esperada.
rmmod en75xx_slic_dxs
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice

dmesg > /tmp/dxs-mailbox-trace.log
dmesg | tail -n 240
```

Se um dos parâmetros não existir, o módulo correspondente ainda não contém
essa alteração. O controlador novo também informa no boot:
`CSI SPI controller ready (status ACK read/OR/write, calibration trace available)`
no EN7523/CSI.

Para desligar os dumps sem descarregar os módulos:

```sh
echo 0 > /sys/module/en75xx_isi_spi/parameters/csi_trace
echo 0 > /sys/module/drv_tapi_dxs/parameters/mailbox_trace
echo 0 > /sys/module/drv_tapi_dxs/parameters/event_trace
```

## Interpretação e validação

- TX diferente do esperado: investigar serialização, header e fragmentação.
- done menor que 6 ou ret diferente de zero: investigar a conclusão do wrapper.
- Frames corretos e done=6/6, sem evento: o estágio de envio pelo wrapper passa;
  isso ainda não separa recepção CSI de falha do firmware/calibração.
- Novo BOOT_FIN: correlacionar com nova consulta de versão; não usar cached_fw
  como prova de firmware ainda em execução.

Quatro testes locais passaram, compilando funções C reais extraídas com
-Wall -Wextra -Werror. Cobrem frames CSI/SPI, padding, RX bruto com byte inicial,
erros, janela limitada, índice inválido, trace desligado, conclusão parcial,
releitura de STATUS e emissão após o payload/fora do spinlock. Não houve build
completo OpenWrt nem teste de hardware nesta etapa. O trace acrescenta tempo
de execução/console e deve permanecer desligado fora dos ensaios.
