import math
import struct
import wave
import os
import random

SAMPLE_RATE = 44100

def write_wav(filename, samples, channels=1):
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with wave.open(filename, 'w') as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        
        # クランプして16bit整数に変換
        raw_bytes = bytearray()
        if channels == 1:
            for s in samples:
                val = max(-1.0, min(1.0, s))
                raw_bytes.extend(struct.pack('<h', int(val * 32767)))
        else:
            for left, right in samples:
                l_val = max(-1.0, min(1.0, left))
                r_val = max(-1.0, min(1.0, right))
                raw_bytes.extend(struct.pack('<hh', int(l_val * 32767), int(r_val * 32767)))
        wf.writeframes(raw_bytes)
    print(f"Generated: {filename} ({len(samples)/SAMPLE_RATE:.2f}s)")

# -------------------------------------------------------------
# 音響波形生成ヘルパー関数
# -------------------------------------------------------------
def square_wave(freq, t, duty=0.5):
    cycle = (t * freq) % 1.0
    return 1.0 if cycle < duty else -1.0

def triangle_wave(freq, t):
    cycle = (t * freq) % 1.0
    if cycle < 0.25:
        return cycle * 4.0
    elif cycle < 0.75:
        return 1.0 - (cycle - 0.25) * 4.0
    else:
        return -1.0 + (cycle - 0.75) * 4.0

def sawtooth_wave(freq, t):
    cycle = (t * freq) % 1.0
    return 2.0 * cycle - 1.0

def sine_wave(freq, t, phase=0.0):
    return math.sin(2.0 * math.pi * freq * t + phase)

# ノート番号から周波数を計算 (MIDI note: 60 = C4 = 261.63Hz)
def note_freq(midi_note):
    return 440.0 * (2.0 ** ((midi_note - 69) / 12.0))

# -------------------------------------------------------------
# 効果音 (SE) の生成
# -------------------------------------------------------------

def gen_se_cursor(path):
    # カーソル移動音：ピピッ（明るい高音サイン＋矩形波の小気味よいクリック）
    duration = 0.04
    total_samples = int(SAMPLE_RATE * duration)
    samples = []
    freq = 1480.0
    for i in range(total_samples):
        t = i / SAMPLE_RATE
        env = math.exp(-t * 80.0)
        s = 0.6 * sine_wave(freq, t) + 0.2 * square_wave(freq, t, 0.25)
        samples.append(s * env * 0.7)
    write_wav(path, samples)

def gen_se_decide(path):
    # 決定音：ピロリン！（明るいメジャー3度〜5度の上昇チャイム）
    notes = [note_freq(72), note_freq(76), note_freq(84)] # C5, E5, C6
    note_dur = 0.05
    samples = []
    for idx, f in enumerate(notes):
        sub_samples = int(SAMPLE_RATE * note_dur)
        if idx == len(notes) - 1:
            sub_samples = int(SAMPLE_RATE * 0.18)
        for i in range(sub_samples):
            t = i / SAMPLE_RATE
            env = math.exp(-t * (12.0 if idx == len(notes) - 1 else 25.0))
            s = 0.6 * sine_wave(f, t) + 0.25 * square_wave(f * 2.0, t, 0.5)
            samples.append(s * env * 0.75)
    write_wav(path, samples)

def gen_se_cancel(path):
    # キャンセル音：ポスッ（下降する柔らかい音）
    duration = 0.08
    total_samples = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(total_samples):
        t = i / SAMPLE_RATE
        f = 520.0 - t * 3000.0
        if f < 100: f = 100
        env = math.exp(-t * 35.0)
        s = 0.7 * sine_wave(f, t) + 0.2 * triangle_wave(f, t)
        samples.append(s * env * 0.7)
    write_wav(path, samples)

def gen_se_pause(path):
    # ポーズ音：近未来風のピロリッ
    notes = [note_freq(67), note_freq(79)] # G4, G5
    samples = []
    for idx, f in enumerate(notes):
        sub_samples = int(SAMPLE_RATE * 0.05)
        if idx == 1:
            sub_samples = int(SAMPLE_RATE * 0.12)
        for i in range(sub_samples):
            t = i / SAMPLE_RATE
            env = math.exp(-t * 20.0)
            s = 0.6 * sine_wave(f, t) + 0.3 * square_wave(f, t, 0.25)
            samples.append(s * env * 0.7)
    write_wav(path, samples)

def gen_se_squat_start(path):
    # 筋トレQTE開始：気合・スライドアップ（シュイーン！）
    duration = 0.14
    total_samples = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(total_samples):
        t = i / SAMPLE_RATE
        f = 220.0 + (t / duration) * 600.0
        env = math.sin(math.pi * (t / duration)) ** 0.5
        noise = (random.random() * 2.0 - 1.0) * 0.15
        s = 0.5 * square_wave(f, t, 0.3) + 0.3 * sine_wave(f * 2, t) + noise
        samples.append(s * env * 0.75)
    write_wav(path, samples)

def gen_se_squat_success(path):
    # 筋トレ成功：シャキーン！＆ レベルアップチャイム（爽快感 MAX）
    # アルペジオ C5 -> G5 -> C6 -> E6 とキラキラした高周波
    notes = [note_freq(72), note_freq(79), note_freq(84), note_freq(88)]
    samples = []
    for idx, f in enumerate(notes):
        dur = 0.045 if idx < len(notes)-1 else 0.28
        sub_samples = int(SAMPLE_RATE * dur)
        for i in range(sub_samples):
            t = i / SAMPLE_RATE
            env = math.exp(-t * (8.0 if idx == len(notes)-1 else 20.0))
            # FM変調でベルのようなシャキーン感
            mod = 3.0 * sine_wave(f * 3.5, t)
            s = 0.6 * sine_wave(f + mod * 100.0, t) + 0.25 * square_wave(f, t, 0.5)
            samples.append(s * env * 0.8)
    write_wav(path, samples)

def gen_se_squat_fail(path):
    # 筋トレ失敗：ブブーッ！（低音不協和音＋筋肉痛脱力）
    duration = 0.35
    total_samples = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(total_samples):
        t = i / SAMPLE_RATE
        # 低音不協和音（130Hz + 138Hz）
        env = math.exp(-t * 6.0)
        s1 = square_wave(130.0, t, 0.5)
        s2 = square_wave(138.0, t, 0.5)
        noise = (random.random() * 2.0 - 1.0) * 0.1
        s = (s1 + s2) * 0.35 + noise
        samples.append(s * env * 0.85)
    write_wav(path, samples)

def gen_se_pounce(path):
    # 飛びつき攻撃：シュバッ！風切り跳躍音
    duration = 0.18
    total_samples = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(total_samples):
        t = i / SAMPLE_RATE
        progress = t / duration
        # 急速にピッチが上がるホワイトノイズとスイープ音
        f = 250.0 + math.sin(progress * math.pi) * 1200.0
        env = math.sin(progress * math.pi)
        noise = (random.random() * 2.0 - 1.0) * 0.5
        s = 0.4 * sine_wave(f, t) + 0.4 * noise
        samples.append(s * env * 0.85)
    write_wav(path, samples)

def gen_se_tackle(path):
    # 電撃タックル：ズガーン！＋雷撃衝撃波（重低音インパクト＋電気ノイズ）
    duration = 0.4
    total_samples = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(total_samples):
        t = i / SAMPLE_RATE
        # バスドラムのようなキック重低音 (180Hz -> 45Hz)
        kick_f = max(40.0, 180.0 * math.exp(-t * 25.0))
        kick = sine_wave(kick_f, t) * math.exp(-t * 8.0)
        # 電気ノイズ
        elec_f = 600.0 + 800.0 * math.sin(t * 120.0)
        elec = square_wave(elec_f, t, 0.2) * math.exp(-t * 14.0) * 0.35
        # 爆発ノイズ
        noise = (random.random() * 2.0 - 1.0) * math.exp(-t * 10.0) * 0.4
        s = kick * 0.6 + elec + noise
        samples.append(s * 0.9)
    write_wav(path, samples)

def gen_se_catch(path):
    # ネズミ捕獲音：ピコーン！＋キラーン（爽快な獲得チャイム）
    notes = [note_freq(79), note_freq(86), note_freq(91)] # G5, D6, G6
    samples = []
    for idx, f in enumerate(notes):
        dur = 0.05 if idx < len(notes)-1 else 0.22
        sub_samples = int(SAMPLE_RATE * dur)
        for i in range(sub_samples):
            t = i / SAMPLE_RATE
            env = math.exp(-t * (10.0 if idx == len(notes)-1 else 22.0))
            s = 0.65 * sine_wave(f, t) + 0.25 * sine_wave(f * 2.0, t) + 0.1 * square_wave(f, t, 0.3)
            samples.append(s * env * 0.8)
    write_wav(path, samples)

def gen_se_rat_squeak(path):
    # ネズミの鳴き声：チュー！キュッ！（高周波FM変調）
    duration = 0.12
    total_samples = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(total_samples):
        t = i / SAMPLE_RATE
        # 2200Hz〜2800Hz前後の急激なピッチ揺れ
        base_f = 2400.0 + 500.0 * math.sin(t * 80.0)
        env = math.sin(math.pi * (t / duration)) ** 1.5
        s = sine_wave(base_f, t) * 0.7 + triangle_wave(base_f * 0.5, t) * 0.2
        samples.append(s * env * 0.7)
    write_wav(path, samples)

def gen_se_pump_decay(path):
    # 筋肉減衰音：ヒューン…（筋肉がしぼむ下降音）
    duration = 0.3
    total_samples = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(total_samples):
        t = i / SAMPLE_RATE
        f = max(80.0, 480.0 - (t / duration) * 360.0)
        env = math.exp(-t * 7.0)
        s = 0.7 * triangle_wave(f, t) + 0.2 * sine_wave(f * 2.0, t)
        samples.append(s * env * 0.75)
    write_wav(path, samples)

def gen_se_stamp(path):
    # ランクスタンプ押印音：ドスン！ドンッ！（重量感あふれる打撃音）
    duration = 0.35
    total_samples = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(total_samples):
        t = i / SAMPLE_RATE
        f = max(35.0, 160.0 * math.exp(-t * 22.0))
        kick = sine_wave(f, t) * math.exp(-t * 7.0)
        punch = (random.random() * 2.0 - 1.0) * math.exp(-t * 30.0) * 0.6
        s = kick * 0.7 + punch
        samples.append(s * 0.9)
    write_wav(path, samples)

def gen_se_countdown(path):
    # カウントダウン警告ビープ：ピッ！
    duration = 0.08
    total_samples = int(SAMPLE_RATE * duration)
    samples = []
    f = 1760.0 # A6
    for i in range(total_samples):
        t = i / SAMPLE_RATE
        env = math.exp(-t * 30.0)
        s = 0.7 * sine_wave(f, t) + 0.2 * square_wave(f, t, 0.5)
        samples.append(s * env * 0.8)
    write_wav(path, samples)

def gen_se_footstep(path):
    # 足音：トコッ、ペタッ（小気味よい肉球タップ音）
    duration = 0.04
    total_samples = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(total_samples):
        t = i / SAMPLE_RATE
        f = max(80.0, 320.0 * math.exp(-t * 60.0))
        env = math.exp(-t * 50.0)
        noise = (random.random() * 2.0 - 1.0) * 0.2
        s = sine_wave(f, t) * 0.6 + noise
        samples.append(s * env * 0.6)
    write_wav(path, samples)

# -------------------------------------------------------------
# BGM (背景音楽) の合成
# -------------------------------------------------------------

def synthesize_bgm_title(path):
    # タイトルBGM: 「マッスルマーチ」
    # 明るく元気、筋トレ前のウォームアップ感。BPM 128, 8小節ループ (4/4拍子)
    bpm = 128.0
    beat_dur = 60.0 / bpm
    total_beats = 32 # 8小節
    total_duration = total_beats * beat_dur
    total_samples = int(SAMPLE_RATE * total_duration)
    
    # ステレオバッファ初期化
    left_ch = [0.0] * total_samples
    right_ch = [0.0] * total_samples
    
    # 進行コード (8小節): C - G - Am - F - C - G - F - G
    chord_roots = [60, 55, 57, 53, 60, 55, 53, 55] # C4, G3, A3, F3...
    
    # 1. ドラムトラック (4つ打ちキック + 2/4拍スネア + 8分ハイハット)
    for b in range(total_beats):
        beat_start_time = b * beat_dur
        start_idx = int(beat_start_time * SAMPLE_RATE)
        
        # キック (毎拍)
        kick_len = int(SAMPLE_RATE * 0.12)
        for i in range(kick_len):
            idx = (start_idx + i) % total_samples
            t = i / SAMPLE_RATE
            f = max(40.0, 160.0 * math.exp(-t * 30.0))
            k = sine_wave(f, t) * math.exp(-t * 12.0) * 0.45
            left_ch[idx] += k
            right_ch[idx] += k
            
        # スネア (拍 1, 3 (0-indexed))
        if b % 2 == 1:
            snare_len = int(SAMPLE_RATE * 0.15)
            for i in range(snare_len):
                idx = (start_idx + i) % total_samples
                t = i / SAMPLE_RATE
                noise = (random.random() * 2.0 - 1.0) * math.exp(-t * 22.0) * 0.3
                tone = triangle_wave(200.0, t) * math.exp(-t * 28.0) * 0.2
                val = noise + tone
                left_ch[idx] += val
                right_ch[idx] += val
                
        # ハイハット (8分音符: 各拍の表と裏)
        for sub in [0.0, 0.5]:
            hh_start = int((beat_start_time + sub * beat_dur) * SAMPLE_RATE)
            hh_len = int(SAMPLE_RATE * 0.04)
            for i in range(hh_len):
                idx = (hh_start + i) % total_samples
                t = i / SAMPLE_RATE
                hh = (random.random() * 2.0 - 1.0) * math.exp(-t * 80.0) * 0.12
                left_ch[idx] += hh * 0.8
                right_ch[idx] += hh * 1.1

    # 2. ベースライン (ファンキーな8分音符ベース)
    for bar in range(8):
        root = chord_roots[bar] - 12 # 1オクターブ下
        # パターン: root, root, 5th, octave
        notes = [root, root, root + 7, root + 12, root, root, root + 7, root]
        for step in range(8):
            note = notes[step]
            step_time = (bar * 4 + step * 0.5) * beat_dur
            start_idx = int(step_time * SAMPLE_RATE)
            dur = beat_dur * 0.4
            sub_len = int(SAMPLE_RATE * dur)
            f = note_freq(note)
            for i in range(sub_len):
                idx = (start_idx + i) % total_samples
                t = i / SAMPLE_RATE
                env = math.exp(-t * 12.0)
                b_val = (0.5 * triangle_wave(f, t) + 0.35 * sine_wave(f, t) + 0.2 * square_wave(f, t, 0.4)) * env * 0.35
                left_ch[idx] += b_val * 0.95
                right_ch[idx] += b_val * 0.95

    # 3. メロディ (キャッチーな筋トレリードシンセ)
    # 8小節のコミカル＆エネルギッシュなメロディライン
    melody_patterns = [
        # Bar 1 (C)
        [(0.0, 0.5, 72), (0.5, 0.5, 74), (1.0, 0.5, 76), (1.5, 0.5, 79), (2.0, 1.0, 84), (3.0, 1.0, 81)],
        # Bar 2 (G)
        [(0.0, 0.5, 79), (0.5, 0.5, 76), (1.0, 1.0, 74), (2.0, 1.0, 74), (3.0, 1.0, 76)],
        # Bar 3 (Am)
        [(0.0, 0.5, 76), (0.5, 0.5, 74), (1.0, 0.5, 72), (1.5, 0.5, 69), (2.0, 1.0, 72), (3.0, 1.0, 76)],
        # Bar 4 (F)
        [(0.0, 1.0, 77), (1.0, 0.5, 76), (1.5, 0.5, 74), (2.0, 2.0, 72)],
        # Bar 5 (C)
        [(0.0, 0.5, 72), (0.5, 0.5, 72), (1.0, 0.5, 76), (1.5, 0.5, 79), (2.0, 1.0, 84), (3.0, 1.0, 86)],
        # Bar 6 (G)
        [(0.0, 0.5, 84), (0.5, 0.5, 81), (1.0, 1.0, 79), (2.0, 1.0, 76), (3.0, 1.0, 74)],
        # Bar 7 (F)
        [(0.0, 0.5, 76), (0.5, 0.5, 77), (1.0, 1.0, 79), (2.0, 1.0, 81), (3.0, 1.0, 83)],
        # Bar 8 (G)
        [(0.0, 1.0, 84), (1.0, 1.0, 83), (2.0, 1.5, 84), (3.5, 0.5, 86)]
    ]
    for bar in range(8):
        bar_start_beat = bar * 4.0
        for offset_beat, dur_beat, note in melody_patterns[bar]:
            start_time = (bar_start_beat + offset_beat) * beat_dur
            start_idx = int(start_time * SAMPLE_RATE)
            sub_len = int(SAMPLE_RATE * (dur_beat * beat_dur * 0.95))
            f = note_freq(note)
            for i in range(sub_len):
                idx = (start_idx + i) % total_samples
                t = i / SAMPLE_RATE
                env = math.exp(-t * (4.0 / dur_beat))
                m_val = (0.5 * square_wave(f, t, 0.25) + 0.3 * sine_wave(f, t)) * env * 0.28
                # 左右パンニング
                left_ch[idx] += m_val * 0.8
                right_ch[idx] += m_val * 1.0

    # 合成と正規化
    combined = []
    for l, r in zip(left_ch, right_ch):
        combined.append((l * 0.9, r * 0.9))
    write_wav(path, combined, channels=2)

def synthesize_bgm_stage(path):
    # ステージBGM: 「疾走！マッスルチェイス」
    # スピード感、緊迫感、爽快感！テンポ 144 BPM, 8小節ループ
    bpm = 144.0
    beat_dur = 60.0 / bpm
    total_beats = 32
    total_duration = total_beats * beat_dur
    total_samples = int(SAMPLE_RATE * total_duration)
    
    left_ch = [0.0] * total_samples
    right_ch = [0.0] * total_samples
    
    # 進行コード (8小節): Dm - Bb - C - F - Dm - Bb - C - A
    chord_roots = [62, 58, 60, 65, 62, 58, 60, 57]
    
    # 1. アップテンポビート
    for b in range(total_beats):
        b_time = b * beat_dur
        start_idx = int(b_time * SAMPLE_RATE)
        # ドライブキック
        k_len = int(SAMPLE_RATE * 0.1)
        for i in range(k_len):
            idx = (start_idx + i) % total_samples
            t = i / SAMPLE_RATE
            f = max(45.0, 200.0 * math.exp(-t * 35.0))
            k = sine_wave(f, t) * math.exp(-t * 14.0) * 0.5
            left_ch[idx] += k
            right_ch[idx] += k
        # スネア (バックビート)
        if b % 2 == 1:
            s_len = int(SAMPLE_RATE * 0.14)
            for i in range(s_len):
                idx = (start_idx + i) % total_samples
                t = i / SAMPLE_RATE
                sn = ((random.random() * 2.0 - 1.0) * 0.35 + square_wave(240.0, t, 0.5) * 0.15) * math.exp(-t * 24.0)
                left_ch[idx] += sn
                right_ch[idx] += sn
        # 16分音符ハイハット
        for sixteenth in range(4):
            hh_time = b_time + sixteenth * (beat_dur / 4.0)
            hh_idx = int(hh_time * SAMPLE_RATE)
            hh_len = int(SAMPLE_RATE * 0.03)
            acc = 1.3 if sixteenth == 2 else 0.8
            for i in range(hh_len):
                idx = (hh_idx + i) % total_samples
                t = i / SAMPLE_RATE
                hh = (random.random() * 2.0 - 1.0) * math.exp(-t * 100.0) * 0.09 * acc
                left_ch[idx] += hh * 1.1
                right_ch[idx] += hh * 0.8

    # 2. ドライビング16分ベース
    for bar in range(8):
        root = chord_roots[bar] - 12
        for step in range(16):
            step_time = (bar * 4.0 + step * 0.25) * beat_dur
            start_idx = int(step_time * SAMPLE_RATE)
            sub_len = int(SAMPLE_RATE * (beat_dur * 0.22))
            # オクターブ跳躍パターン
            note = root + (12 if step % 2 == 1 else 0)
            f = note_freq(note)
            for i in range(sub_len):
                idx = (start_idx + i) % total_samples
                t = i / SAMPLE_RATE
                env = math.exp(-t * 18.0)
                b_val = (0.5 * sawtooth_wave(f, t) + 0.4 * sine_wave(f, t)) * env * 0.32
                left_ch[idx] += b_val
                right_ch[idx] += b_val

    # 3. 爽快な疾走メロディ (リードシンセ)
    melody_notes = [
        # Bar 1 (Dm)
        [(0.0, 0.5, 74), (0.5, 0.5, 77), (1.0, 0.5, 81), (1.5, 0.5, 84), (2.0, 1.0, 81), (3.0, 1.0, 77)],
        # Bar 2 (Bb)
        [(0.0, 0.5, 77), (0.5, 0.5, 82), (1.0, 1.0, 81), (2.0, 1.0, 77), (3.0, 1.0, 74)],
        # Bar 3 (C)
        [(0.0, 0.5, 76), (0.5, 0.5, 79), (1.0, 0.5, 84), (1.5, 0.5, 83), (2.0, 1.0, 79), (3.0, 1.0, 76)],
        # Bar 4 (F)
        [(0.0, 1.0, 81), (1.0, 0.5, 79), (1.5, 0.5, 77), (2.0, 2.0, 76)],
        # Bar 5 (Dm)
        [(0.0, 0.5, 74), (0.5, 0.5, 77), (1.0, 0.5, 81), (1.5, 0.5, 86), (2.0, 1.0, 84), (3.0, 1.0, 81)],
        # Bar 6 (Bb)
        [(0.0, 0.5, 82), (0.5, 0.5, 84), (1.0, 1.0, 86), (2.0, 1.0, 84), (3.0, 1.0, 82)],
        # Bar 7 (C)
        [(0.0, 0.5, 84), (0.5, 0.5, 86), (1.0, 1.0, 88), (2.0, 1.0, 86), (3.0, 1.0, 84)],
        # Bar 8 (A)
        [(0.0, 1.0, 85), (1.0, 1.0, 84), (2.0, 2.0, 86)]
    ]
    for bar in range(8):
        bar_start_beat = bar * 4.0
        for offset_beat, dur_beat, note in melody_notes[bar]:
            start_time = (bar_start_beat + offset_beat) * beat_dur
            start_idx = int(start_time * SAMPLE_RATE)
            sub_len = int(SAMPLE_RATE * (dur_beat * beat_dur * 0.92))
            f = note_freq(note)
            for i in range(sub_len):
                idx = (start_idx + i) % total_samples
                t = i / SAMPLE_RATE
                env = math.exp(-t * (3.5 / dur_beat))
                m_val = (0.5 * square_wave(f, t, 0.5) + 0.3 * sawtooth_wave(f, t)) * env * 0.28
                left_ch[idx] += m_val * 0.85
                right_ch[idx] += m_val * 1.05

    combined = []
    for l, r in zip(left_ch, right_ch):
        combined.append((l * 0.88, r * 0.88))
    write_wav(path, combined, channels=2)

def synthesize_bgm_clear(path):
    # ゲームクリアファンファーレ: 勝利の讃歌！
    # テンポ 120 BPM, 約5秒の豪華ジングル
    bpm = 120.0
    beat_dur = 60.0 / bpm
    total_duration = 5.2
    total_samples = int(SAMPLE_RATE * total_duration)
    
    left_ch = [0.0] * total_samples
    right_ch = [0.0] * total_samples
    
    # ファンファーレのメロディと和音
    fanfare = [
        # (beat, dur, [notes])
        (0.0, 0.25, [67, 72]),
        (0.25, 0.25, [67, 72]),
        (0.5, 0.25, [67, 72]),
        (0.75, 0.75, [72, 76, 79]),
        (1.5, 0.25, [74, 77, 81]),
        (1.75, 0.75, [76, 79, 84]),
        (2.5, 0.5, [74, 77, 81]),
        (3.0, 1.8, [76, 79, 84, 88])
    ]
    for offset_beat, dur_beat, chord in fanfare:
        start_time = offset_beat * beat_dur
        start_idx = int(start_time * SAMPLE_RATE)
        dur = dur_beat * beat_dur
        sub_len = int(SAMPLE_RATE * dur)
        for note in chord:
            f = note_freq(note)
            for i in range(sub_len):
                idx = start_idx + i
                if idx >= total_samples: break
                t = i / SAMPLE_RATE
                env = math.exp(-t * (2.2 / dur_beat))
                val = (0.5 * square_wave(f, t, 0.3) + 0.35 * sine_wave(f, t)) * env * 0.2
                left_ch[idx] += val
                right_ch[idx] += val
                
    # ティンパニ/シンバル風のインパクト
    for crash_beat in [0.75, 1.75, 3.0]:
        c_time = crash_beat * beat_dur
        c_idx = int(c_time * SAMPLE_RATE)
        c_len = int(SAMPLE_RATE * 1.2)
        for i in range(c_len):
            idx = c_idx + i
            if idx >= total_samples: break
            t = i / SAMPLE_RATE
            noise = (random.random() * 2.0 - 1.0) * math.exp(-t * 6.0) * 0.25
            kick = sine_wave(max(40.0, 140.0 * math.exp(-t * 20.0)), t) * math.exp(-t * 8.0) * 0.4
            left_ch[idx] += kick + noise * 0.8
            right_ch[idx] += kick + noise * 1.1

    combined = []
    for l, r in zip(left_ch, right_ch):
        combined.append((l * 0.85, r * 0.85))
    write_wav(path, combined, channels=2)

def synthesize_bgm_timeup(path):
    # タイムアップジングル: 脱力・コミカルな終了音（「ボヨヨ〜ン…」）
    duration = 3.5
    total_samples = int(SAMPLE_RATE * duration)
    left_ch = [0.0] * total_samples
    right_ch = [0.0] * total_samples
    
    # 下降する不協和音
    chords = [
        (0.0, 0.5, [72, 75]),
        (0.5, 0.5, [71, 74]),
        (1.0, 0.5, [70, 73]),
        (1.5, 1.8, [69, 72, 65])
    ]
    for start_time, dur, chord in chords:
        start_idx = int(start_time * SAMPLE_RATE)
        sub_len = int(SAMPLE_RATE * dur)
        for note in chord:
            f = note_freq(note)
            for i in range(sub_len):
                idx = start_idx + i
                if idx >= total_samples: break
                t = i / SAMPLE_RATE
                env = math.exp(-t * 3.0)
                # ピッチベンド下降
                bend_f = f * math.exp(-t * 0.6)
                val = triangle_wave(bend_f, t) * env * 0.25
                left_ch[idx] += val
                right_ch[idx] += val
                
    # ボヨヨン効果
    boing_idx = int(1.5 * SAMPLE_RATE)
    boing_len = int(SAMPLE_RATE * 1.8)
    for i in range(boing_len):
        idx = boing_idx + i
        if idx >= total_samples: break
        t = i / SAMPLE_RATE
        f = 140.0 + 40.0 * math.sin(t * 35.0) * math.exp(-t * 2.0)
        env = math.exp(-t * 2.5)
        s = sine_wave(f, t) * env * 0.35
        left_ch[idx] += s
        right_ch[idx] += s

    combined = []
    for l, r in zip(left_ch, right_ch):
        combined.append((l * 0.85, r * 0.85))
    write_wav(path, combined, channels=2)

def main():
    sounds_dir = r"D:\MuscleCat\Resource\Sounds"
    print("Generating SE...")
    gen_se_cursor(os.path.join(sounds_dir, "se_cursor.wav"))
    gen_se_decide(os.path.join(sounds_dir, "se_decide.wav"))
    gen_se_cancel(os.path.join(sounds_dir, "se_cancel.wav"))
    gen_se_pause(os.path.join(sounds_dir, "se_pause.wav"))
    gen_se_squat_start(os.path.join(sounds_dir, "se_squat_start.wav"))
    gen_se_squat_success(os.path.join(sounds_dir, "se_squat_success.wav"))
    gen_se_squat_fail(os.path.join(sounds_dir, "se_squat_fail.wav"))
    gen_se_pounce(os.path.join(sounds_dir, "se_pounce.wav"))
    gen_se_tackle(os.path.join(sounds_dir, "se_tackle.wav"))
    gen_se_catch(os.path.join(sounds_dir, "se_catch.wav"))
    gen_se_rat_squeak(os.path.join(sounds_dir, "se_rat_squeak.wav"))
    gen_se_pump_decay(os.path.join(sounds_dir, "se_pump_decay.wav"))
    gen_se_stamp(os.path.join(sounds_dir, "se_stamp.wav"))
    gen_se_countdown(os.path.join(sounds_dir, "se_countdown.wav"))
    gen_se_footstep(os.path.join(sounds_dir, "se_footstep.wav"))

    print("\nGenerating BGM...")
    synthesize_bgm_title(os.path.join(sounds_dir, "bgm_title.wav"))
    synthesize_bgm_stage(os.path.join(sounds_dir, "bgm_stage.wav"))
    synthesize_bgm_clear(os.path.join(sounds_dir, "bgm_clear.wav"))
    synthesize_bgm_timeup(os.path.join(sounds_dir, "bgm_timeup.wav"))

    print("\nAll sounds generated successfully!")

if __name__ == '__main__':
    main()
