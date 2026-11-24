=== ゲームプレイ画面 HUD カスタム PNG 配置フォルダ ===

このフォルダ（Resource/UI/）に以下のファイル名で PNG 画像を置くと、
ゲームプレイ画面の UI に自動で読み込まれて表示されます（透過PNG対応）。
画像が存在しない場合は、従来の標準UIがそのまま表示されます。

1. hud_status.png   : 左上ステータスHUDの背景フレーム（標準サイズ: 380x84）
2. hud_guide.png    : 右下操作ガイドHUDの背景フレーム（標準サイズ: 640x72）
3. hud_overlay.png  : 画面に追加表示する自由イラスト用オーバーレイ（任意）

【座標・サイズの変更方法】
Source/Config.h の `namespace HudUI` にある
STATUS_X / Y / W / H、GUIDE_X / Y / W / H、OVERLAY_X / Y / W / H
を書き換えるだけで位置とサイズを自由に変更できます。
（※ W または H を 0 にすると、PNG画像の元サイズそのまま描画されます）
