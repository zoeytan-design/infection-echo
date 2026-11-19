/*
  衡評述值 Pricetory — 敲槌按鈕程式（只管按鈕）
  ------------------------------------------------------------
  按一下按鈕＝敲槌，傳 HIT 給電腦。

  傳給電腦的訊息：
    READY   開機完成
    HIT     按下按鈕（敲槌）
    DBG ... / INFO ...  給人看的狀況說明（主機程式會忽略）

  ===== 接線（Nano）=====
    3 隻腳的按鈕模組：
      VCC  →  5V
      GND  →  GND
      IN   →  D2      （模組上寫 IN / OUT / S 都是這隻）
    2 隻腳（或 4 隻腳）的一般按鈕：
      一隻腳 → D2、另一隻腳 → GND（4 隻腳的用對角兩隻）

  ⚠ 插 USB 或按重置鍵的時候「不要按著按鈕」：
     程式開機時會記住「沒按」是什麼狀態，所以按下變 HIGH 或變 LOW 的模組都能用。

  ===== 燒錄（Nano）=====
    工具 → 開發板 → Arduino Nano
    工具 → 處理器 → ATmega328P（上傳失敗改選「ATmega328P (Old Bootloader)」）
    監控視窗速度：115200
*/

// ======== 可以調整的設定 ========
const byte BTN_PIN = 2;                   // 按鈕接哪隻腳
const unsigned long DEBOUNCE_MS = 30;     // 狀態要穩定多久才算數（去掉按鈕接觸時的抖動）
const unsigned long COOLDOWN_MS = 1200;   // 按一次之後，多久內不算第二次（避免連按）
#define DEBUG 1                           // 1：印出「按下」等說明；正式展出可以改 0
// =================================

int idleLevel = HIGH;          // 沒按的時候讀到的值（開機時自動記下來）
int rawLevel = HIGH, stableLevel = HIGH;
unsigned long changedAt = 0, lastHit = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);          // 開內建上拉電阻：一般按鈕不用另外接電阻
  delay(50);
  idleLevel = rawLevel = stableLevel = digitalRead(BTN_PIN);
#if DEBUG
  Serial.print(F("INFO button pin D")); Serial.print(BTN_PIN);
  Serial.print(F(" idle=")); Serial.print(idleLevel);
  Serial.println(idleLevel == HIGH ? F("（按下會變 0）") : F("（按鈕模組：按下會變 1）"));
#endif
  Serial.println("READY");
}

void loop() {
  int r = digitalRead(BTN_PIN);
  if (r != rawLevel) { rawLevel = r; changedAt = millis(); }   // 有變化 → 重新計時
  if (millis() - changedAt < DEBOUNCE_MS || r == stableLevel) return;
  stableLevel = r;                                              // 穩定了，才算真的改變

  if (stableLevel != idleLevel) {                               // 按下去的那一瞬間
    if (millis() - lastHit < COOLDOWN_MS) {
#if DEBUG
      Serial.println(F("DBG BTN 按太快了，這次不算"));
#endif
      return;
    }
    lastHit = millis();
#if DEBUG
    Serial.println(F("DBG BTN ✓ 按下 → 敲槌"));
#endif
    Serial.println("HIT");
  }
}
