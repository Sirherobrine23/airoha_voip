# DXS: observar polling e registradores HOST após falha de calibração

Aplicar depois dos patches stage-isolation e oem-hostcfg. Este é um patch
de diagnóstico, não uma correção da calibração.

## Resultado que motivou o diagnóstico

O log `14-14-21_1792170861.log` confirma HOST_CFG solicitado/lido=0x07 e
firmware 2.1.1 ativo depois de DEV_START e do BBD. Em 62.627962 s é enviado
opmode=6. A calibração expira com pending=1, sem novo evento de modo.
A consulta de versão também expira. Não há novo BOOT_FIN nem leitura de ROM
depois desse comando. O ajuste de polaridade ao OEM não resolveu essa tentativa.

O trace anterior só imprimia INT1 quando diferente de zero ou quando a
leitura retornava erro. Ausência de mensagens não distinguia polling parado
de polling ativo lendo INT1=0. E a consulta de versão depende de uma resposta
de firmware pela mailbox, não somente de uma leitura SPI de registrador.

## Instrumentação

Quando event_trace=1, o backend conta etapas por dispositivo:

| Campo | Ponto observado |
|---|---|
| poll | Callback periódico encontrou o dispositivo |
| eligible | Modo polling e DS_BASIC_INIT permitem chamar o handler |
| handler | Handler passou pelos testes de SPI ativo e modo não STOP |
| int_reads | A leitura normal de INT1/INT2 retornou, com sucesso ou erro |
| outbox=A/B | Entradas/retornos do processamento da outbox |
| done | Handler terminou o caminho ativo |

Os contadores são atômicos e cumulativos desde o carregamento do backend,
contando somente enquanto event_trace estiver habilitado. Não são uma captura
atômica de todos os campos: o polling pode avançar entre as leituras.

Uma amostra é impressa antes de iniciar a calibração, e outra se a espera
falhar. Cada amostra também lê os seguintes registradores do próprio DXS:

| Offset | Registro |
|---|---|
| 0x06 | HOST_CFG |
| 0x08 | HOST_IEN1 |
| 0x0a | HOST_IEN2 |
| 0x18 | HOST_LEN |
| 0x24 | HOST_BINF |

Esses offsets não são MMIO do SoC para acesso com devmem. As leituras usam
o transporte existente. O observador NÃO lê os registradores INT1/INT2,
cujos indicadores podem limpar na leitura, nem DATA, que retira conteúdo
da outbox. O handler normal continua sendo responsável por esses acessos.
Nenhum comando de recuperação, reset, linefeed ou calibração adicional é enviado.

A linha `cached_fw` mostra o cache do host e não prova qual firmware está
executando após a falha. Cada leitura tem um marcador anterior `reading reg=`
e depois valor/status ou erro. Se não retornar, o último marcador ajuda a
localizar o acesso, mas não determina sozinho a causa do bloqueio.

As amostras adicionam leituras e mensagens de console; isso altera o timing
da observação. O timeout original não é aumentado e nenhum resultado de
calibração é forçado. O patch pode ser desativado com event_trace=0.

## Aplicação e teste

```sh
git apply --check /caminho/airoha-voip-dxs-host-observer.patch
git apply /caminho/airoha-voip-dxs-host-observer.patch
```

Recompile e instale `drv_tapi_dxs.ko` para o kernel da placa. Não há símbolo
novo exigido pelo adaptador. Mantenha firmware, BBD, timing PCM e HOST_CFG do
último teste. Depois de substituir o backend, com ambos os módulos carregados:

```sh
rmmod en75xx_slic_dxs &&
rmmod drv_tapi_dxs &&
modprobe drv_tapi_dxs &&
echo 1 > /sys/module/drv_tapi_dxs/parameters/event_trace &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice

dmesg | tail -n 200
```

Se os módulos não estiverem carregados, omita os rmmod correspondentes.

## Interpretar comparando as duas amostras

- poll/eligible/handler/int_reads/done avançam, outbox não avança e as leituras
  HOST continuam coerentes: há progresso do processamento no host e retorno
  de transações de registrador. Investigar ausência de resposta/eventos do
  firmware e validade dos dados CSI. Isso não prova sozinho que o chip está saudável.
- poll não avança: investigar timer, callback, workqueue, retorno por reboot,
  disponibilidade do dispositivo ou bloqueio dentro da execução anterior.
- poll avança, eligible não: conferir mode/state; o filtro não permite o handler.
- handler avança mas int_reads não: investigar o acesso normal aos registradores
  e seu transporte. Um acesso em andamento pode produzir diferença transitória.
- outbox A avança sem B acompanhar: investigar processamento da outbox,
  locks e acessos realizados dentro dele.
- registradores passam a 0/0xffff ou retornam erro: comparar com o baseline,
  CSI/reset/clocks/alimentação. status=0 no SPI não é uma confirmação física
  de presença do SLIC: alguns barramentos retornam dados flutuantes sem erro.

O padrão mais recente não demonstra volta à ROM. Somente uma consulta válida
de versão ou evidência de boot permite afirmar isso; nunca usar cached_fw
como substituto da leitura real.

## Verificação local

`python3 -m unittest discover -s tests -p 'test_dxs_host_observer.py' -v`
compila trechos C reais com registros/tempo simulados para os caminhos de
acesso de 8 e 16 bits. Cobre polling ativo/inelegível, SPI inativo, outbox,
erros, trace desabilitado e a lista exata de registradores das amostras.
Passou localmente; ainda requer build completo OpenWrt e execução na placa.
