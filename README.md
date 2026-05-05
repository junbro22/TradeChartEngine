# TradeChartEngine

C++로 작성된 증권/암호화폐 차트 엔진 + iOS · Android · Web 바인딩.

## 책임 분리

```
┌──────────────── Host App ────────────────┐
│  WebSocket / REST · 인증 · 재연결         │
│  ↓ 받은 데이터를 엔진에 push              │
└──────────────────┬───────────────────────┘
                   ▼
┌──────────── TradeChartEngine ────────────┐
│  데이터 모델 · 지표 계산 · 메시 빌드       │
│  뷰포트 · 인터랙션 좌표 변환                │
│  (네트워크 코드 0줄)                       │
└──────────────────┬───────────────────────┘
                   ▼
       Native GPU API (Metal · GLES · WebGL)
```

엔진은 **순수 시각화 라이브러리**다. 호스트 앱이 자신의 WebSocket/REST 클라이언트로 받은 데이터를 `chart_append_candle()` 등으로 push하면 엔진이 vertex/index 버퍼를 만들어 돌려준다. GPU draw는 wrapper가 native API로 처리.

## 디렉토리

```
.
├── core/                    C++ 엔진 (CMake)
│   ├── include/tce/         public C ABI 헤더 (extern "C")
│   ├── src/                 구현 (data, indicator, renderer, viewport)
│   └── tests/               호스트 단위 테스트
├── bindings/
│   └── ios/                 Swift Package + xcframework 빌드 스크립트
└── examples/
    └── ios-demo/            iOS Tuist 샘플 앱 (Metal 렌더)
```

## Phase 1 스코프 (현재)

- 캔들스틱 + 라인 차트
- SMA / EMA
- 거래량 보조 패널
- 줌 / 팬 / 크로스헤어
- iOS xcframework + Swift Package + Sample 앱

## Phase 2~ (예정)

핵심 지표(RSI/MACD/Bollinger Bands/Stochastic/ATR), 일목균형표, 피보나치 되돌림, 추세선 드로잉, 매수/매도 마커, Android/Web 바인딩.

## 빌드

```bash
# Host 단위 테스트
cmake --preset host
ctest --preset host

# iOS xcframework
./bindings/ios/build_xcframework.sh
```

## 라이선스

추후 명시.
