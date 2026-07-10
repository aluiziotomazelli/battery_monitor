# Code Review Report — `battery_monitor` Component

> **Date:** 2026-07-09
> **Scope:** Full static review of `src/`, `include/`, `host_test/`
> **Reviewer:** Antigravity (AI-assisted)

---

## Pontos Fortes

### Arquitetura e Design
- **HAL abstrato e testável**: Todas as dependências de hardware (ADC, calibração, timer) são completamente abstraídas via interfaces ABC. Isso permite testes unitários de host sem nenhum hardware real.
- **Dependency Injection por construtor**: `AdcBatteryReader` e `BatteryMonitor` recebem todas as dependências via construtor, eliminando estado global e acoplamento implícito.
- **SRP bem respeitado**: `AdcBatteryReader` cuida exclusivamente da leitura do hardware; `BatteryMonitor` cuida exclusivamente da lógica de negócio (divisor, percentual, estado). Não há mistura de responsabilidades.
- **Fallback gracioso de calibração**: Se `adc_cali_create_scheme_curve_fitting()` falhar (eFuse não programado, hardware incompatível), o componente continua operando com conversão linear. A calibração é "best-effort" sem impacto na inicialização.
- **Cleanup em falha de `init()`**: Se `config_channel()` falhar, `del_unit()` é chamado antes de retornar, evitando resource leak do handle do ADC.
- **Compilação condicional para host**: Fake types de `esp_adc` dentro de `#if defined(__linux__)` permitem compilar toda a lógica de negócio no host sem depender de headers do IDF target.
- **Cobertura de testes**: 26 testes cobrindo 96.7% das linhas de produção e 100% das funções públicas. Paths de erro críticos (falha de init, falha de leitura, divisor zero) todos testados.

### Qualidade de Código
- **Prevenção de double-init/deinit**: Ambas as classes verificam `initialized_` no início de `init()`, `deinit()` e `read()` e retornam `ESP_ERR_INVALID_STATE` imediatamente.
- **Overflow controlado no percentual**: O cálculo de percentual faz upcast para `uint32_t` antes da multiplicação por 100, evitando overflow com valores de tensão próximos dos limites.
- **Delay apenas entre amostras**: O loop de amostragem chama `delay_us()` somente entre amostras, não após a última — comportamento correto que evita latência desnecessária.

---

## Pontos Fracos e Possíveis Melhorias

### [MEDIUM] 1. Validação de configuração atrasada

**Arquivo:** `src/battery_monitor.cpp:71`

`divider_bottom_ohms == 0` é verificado dentro de `read()`, mas não em `init()` ou no construtor.

```cpp
// Comportamento atual: só falha em runtime na primeira leitura
if (config_.divider_bottom_ohms == 0) {
    return ESP_ERR_INVALID_ARG;  // tarde demais
}
```

Outras inconsistências de configuração também não são verificadas:
- `full_mv <= empty_mv` → divisão por zero no cálculo de percentual
- `critical_mv > low_mv` → estados mal ordenados, classificação incorreta
- `sample_count == 0` → tratado com fallback para 1, mas silenciosamente

**Sugestão:** Adicionar um método `validate_config()` chamado em `init()` que verifique todas as invariantes da configuração de uma vez, retornando `ESP_ERR_INVALID_ARG` com log descritivo.

```cpp
// Proposta
static esp_err_t validate_config(const BatteryMonitorConfig& cfg) {
    if (cfg.divider_bottom_ohms == 0)       return ESP_ERR_INVALID_ARG;
    if (cfg.full_mv <= cfg.empty_mv)        return ESP_ERR_INVALID_ARG;
    if (cfg.critical_mv >= cfg.low_mv)      return ESP_ERR_INVALID_ARG;
    if (cfg.low_mv >= cfg.full_mv)          return ESP_ERR_INVALID_ARG;
    return ESP_OK;
}
```

---

### [MEDIUM] 2. Configuração do clock source opaca

**Arquivo:** `src/adc_battery_reader.cpp` (init)

```cpp
// Atual: valor inteiro literal sem semântica
.clk_src = static_cast<adc_oneshot_clk_src_t>(0),
```

O cast para `0` funciona porque o ESP-IDF internamente trata `0` como "default clock source", mas isso é um detalhe de implementação não documentado.

**Sugestão:** Usar a constante nominal do ESP-IDF se disponível para o target:

```cpp
// ESP-IDF 5.x define ADC_RTC_CLK_SRC_DEFAULT para ESP32-C3
.clk_src = ADC_RTC_CLK_SRC_DEFAULT,  // se disponível para o target
// ou, documentar explicitamente o motivo do 0:
// .clk_src = static_cast<adc_oneshot_clk_src_t>(0), // 0 = ADC_RTC_CLK_SRC_DEFAULT
```

> **Nota:** A disponibilidade de `ADC_RTC_CLK_SRC_DEFAULT` varia por chip e versão do IDF. Requer consulta à documentação do target em uso.

---

### [LOW] 3. `get_time_us()` na `IBmHalTimer` sem uso

**Arquivo:** `include/interfaces/i_bm_hal_timer.hpp`

O método `get_time_us()` está declarado na interface e implementado em `BmHalTimer` e `MockBmHalTimer`, mas não é chamado em nenhum ponto de `AdcBatteryReader` ou `BatteryMonitor`.

**Trade-off:** Manter a função na interface é defensivo — ela pode ser útil no futuro para implementar timeout no loop de amostragem (ver ponto 4) ou para timestamps de log. O custo é mínimo.

**Opções:**
- **Manter** como preparação para melhoria futura do watchdog de timeout.
- **Remover** agora e readicionar quando houver caso de uso concreto, seguindo YAGNI.

---

### [LOW] 4. Ausência de timeout no loop de amostragem

**Arquivo:** `src/adc_battery_reader.cpp` (read_adc_mv)

`timer_hal_.delay_us()` é chamado de forma bloqueante `(sample_count - 1)` vezes. Com a configuração padrão (16 amostras, 1000 µs de delay), o tempo total de bloqueio é ~15 ms.

```
Tempo máximo do loop = (sample_count - 1) * sample_delay_us
                     = 15 * 1000 µs = 15 ms
```

**Por que é baixo risco:** `adc_oneshot_read()` é síncrono e extremamente rápido no ESP-IDF (~10–100 µs). Não há operação de I/O, DMA ou tarefa bloqueante envolvida. Um travamento do ADC seria um evento catastrófico de hardware, não um cenário de timeout.

**Potencial de melhoria:** Se `get_time_us()` fosse usado, seria possível implementar um timeout por amostra individual:

```cpp
// Exemplo conceitual
int64_t deadline = timer_hal_.get_time_us() + SAMPLE_TIMEOUT_US;
// ... lógica de verificação de deadline
```

Na prática, para um dispositivo de deep sleep cíclico como este, o risco real é negligenciável.

---

### [LOW] 5. Ausência de histerese nos thresholds de estado

**Arquivo:** `src/battery_monitor.cpp:94-105`

Os thresholds de classificação de estado (`CRITICAL`, `LOW`, `NORMAL`, `FULL`) são verificados sem histerese. Um valor oscilando em torno de `low_mv` (ex: 3400 mV) causaria alternância entre `LOW` e `NORMAL` a cada ciclo.

**Trade-off documentado:** Em dispositivos com deep sleep cíclico, o estado anterior não é preservado entre ciclos sem uso de NVS ou RTC memory. Implementar histerese stateful requereria uma das seguintes abordagens:

| Abordagem | Custo | Persistência |
|---|---|---|
| RTC memory (`RTC_DATA_ATTR`) | Baixo | Perde no power-off total |
| NVS | Médio | Persiste em todos os cenários |
| Histerese em banda sem estado | Zero | Imune à oscilação por definição |

**Histerese sem estado (proposta simples):**
```cpp
// Margem configurável na BatteryMonitorConfig
static constexpr uint16_t HYSTERESIS_MV = 50;
if (voltage_mv <= critical_mv)                   state = CRITICAL;
else if (voltage_mv <= low_mv + HYSTERESIS_MV)   state = LOW;
else if (voltage_mv >= full_mv)                  state = FULL;
else                                             state = NORMAL;
```
Essa abordagem não requer estado persistido, mas exige que a margem `HYSTERESIS_MV` seja configurável.

---

### [LOW] 6. `out.adc_mv` preenchido antes da verificação do divisor

**Arquivo:** `src/battery_monitor.cpp:68-74`

```cpp
out.adc_mv = adc_mv;                         // (linha 68) campo preenchido
if (config_.divider_bottom_ohms == 0) {       // (linha 71) verificação tardia
    return ESP_ERR_INVALID_ARG;               // saída com struct parcialmente preenchida
}
```

Em caso de erro, `out` retorna com `adc_mv` preenchido mas `voltage_mv`, `percent` e `state` nos valores default. O chamador pode interpretar erroneamente uma struct com dados parciais válidos.

**Sugestão:** Verificar o divisor antes de qualquer escrita em `out`. Com a correção do ponto #1 (validação em `init()`), esse ponto deixa de ser relevante pois o erro seria capturado antes do `read()`.

---

## Resumo Executivo

| # | Ponto | Severidade | Complexidade de Correção |
|---|---|---|---|
| 1 | Validação de config atrasada (divisor zero, thresholds inválidos) | Medium | Baixa |
| 2 | Clock source ADC opaco (cast `0`) | Medium | Baixa |
| 3 | `get_time_us()` sem uso na interface | Low | Baixa |
| 4 | Sem timeout no loop de amostragem | Low | Média |
| 5 | Ausência de histerese nos thresholds | Low | Média |
| 6 | Struct parcialmente preenchida em erro de divisor | Low | Baixa (resolvido pelo #1) |

Os pontos de maior custo-benefício para correção imediata são o **#1** (validação em `init()`) e o **#2** (documentar/clarificar o clock source), pois têm baixa complexidade e melhoram diretamente a robustez e legibilidade do código.
