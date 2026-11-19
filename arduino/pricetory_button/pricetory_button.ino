/*
  衡評述值 Pricetory — A / B 成交按鈕程式（兩顆按鈕）
  ------------------------------------------------------------
  原本：一顆按鈕 = 敲槌（傳 HIT）
  現在：兩顆按鈕 = 選擇「A 號 AI」或「B 號 AI」成交

  傳給電腦的訊息：
    READY    開機完成
    PICK_A   按下 A 按鈕（選 A 號 AI 成交）
    PICK_B   按下 B 按鈕（選 B 號 AI 成交）
    DBG ... / INFO ...  給人看的狀況說明（主機程式請忽略）

  ===== 接線（Nano）=====
    A 按鈕 → D2
    B 按鈕 → D3

    3 隻腳的按鈕模組（每顆都一樣接法）：
      VCC → 5V
      GND → GND
      IN  → D2（A）或 D3（B）

    2 隻腳（或 4 隻腳）的一般按鈕：
      一隻腳 → D2（A）或 D3（B）
      另一隻腳 → GND（4 隻腳的用對角兩隻）

  ⚠ 插 USB 或按重置鍵的時候「不要按著按鈕」：
     程式開機時會記住「沒按」是什麼狀態，所以按下變 HIGH 或變 LOW 的模組都能用。

  ===== 燒錄（Nano）=====
    工具 → 開發板 → Arduino Nano
    工具 → 處理器 → ATmega328P（上傳失敗改選「ATmega328P (Old Bootloader)」）
    監控視窗速度：115200
*/

// ======== 可以調整的設定 ========
const byte BTN_PIN[2] = {2, 3};           // 第 0 顆＝A 接 D2，第 1 顆＝B 接 D3
const char LABEL[2]   = {'A', 'B'};       // 傳給電腦時用的名字
const unsigned long DEBOUNCE_MS = 30;     // 狀態要穩定多久才算數（去掉按鈕接觸時的抖動）
const unsigned long COOLDOWN_MS = 1200;   // 按下任何一顆之後，多久內不算第二次（避免連按、避免 A B 同時按）
#define DEBUG 1                           // 1：印出「按下」等說明；正式展出可以改 0
// =================================

// 兩顆按鈕各自需要的狀態，用陣列存：[0] 是 A、[1] 是 B
int idleLevel[2];                 // 沒按的時候讀到的值（開機時自動記下來）
int rawLevel[2];                  // 最新讀到的值
int stableLevel[2];               // 穩定後確認的值
unsigned long changedAt[2] = {0, 0};

unsigned long lastPick = 0;       // 上一次有效按下的時間（兩顆共用）
bool hasPicked = false;           // 開機後是否已經按過一次

void setup() {
  Serial.begin(115200);
  for (byte i = 0; i < 2; i++) {
    pinMode(BTN_PIN[i], INPUT_PULLUP);   // 開內建上拉電阻：一般按鈕不用另外接電阻
  }
  delay(50);
  for (byte i = 0; i < 2; i++) {
    idleLevel[i] = rawLevel[i] = stableLevel[i] = digitalRead(BTN_PIN[i]);
#if DEBUG
    Serial.print(F("INFO button "));
    Serial.print(LABEL[i]);
    Serial.print(F(" pin D"));
    Serial.print(BTN_PIN[i]);
    Serial.print(F(" idle="));
    Serial.println(idleLevel[i]);
#endif
  }
  Serial.println("READY");
}

void loop() {
  // 每一圈，依序檢查 A、B 兩顆按鈕
  for (byte i = 0; i < 2; i++) {
    int r = digitalRead(BTN_PIN[i]);

    if (r != rawLevel[i]) {                       // 有變化 → 重新計時
      rawLevel[i] = r;
      changedAt[i] = millis();
    }
    if (millis() - changedAt[i] < DEBOUNCE_MS || r == stableLevel[i]) continue;
    stableLevel[i] = r;                           // 穩定了，才算真的改變

    if (stableLevel[i] != idleLevel[i]) {         // 按下去的那一瞬間
      if (hasPicked && millis() - lastPick < COOLDOWN_MS) {
#if DEBUG
        Serial.print(F("DBG BTN "));
        Serial.print(LABEL[i]);
        Serial.println(F(" 按太快了，這次不算"));
#endif
        continue;
      }
      lastPick = millis();
      hasPicked = true;
#if DEBUG
      Serial.print(F("DBG BTN "));
      Serial.print(LABEL[i]);
      Serial.println(F(" 按下 → 選擇成交"));
#endif
      Serial.print("PICK_");
      Serial.println(LABEL[i]);                   // 傳 PICK_A 或 PICK_B
    }
  }
}
