let data = typeof msg.payload === "string" ? JSON.parse(msg.payload) : msg.payload;

// Parse numeric values
["bmp", "gas", "mpu_x", "mpu_y", "mpu_z", "kompas", "lat", "lon"].forEach((key) => {
  data[key] = parseFloat(data[key]);
});
data.shock = parseInt(data.shock);

// --- STRATEGI BPM ---
let strategi_bpm = 1; // Default: tidak normal
if (data.bmp === 0 || data.bmp < 20) strategi_bpm = 2; // Tidak ada detak
else if (data.bmp >= 60 && data.bmp <= 100) strategi_bpm = 0; // Normal

// --- STRATEGI GAS ---
let strategi_gas = 0;
if (data.gas >= 300) strategi_gas = 2;
else if (data.gas >= 100) strategi_gas = 1;

// --- STRATEGI SHOCK ---
let strategi_shock = data.shock === 1 ? 1 : 0;

// --- STRATEGI GABUNGAN GAS + BPM (opsional, tetap dipertahankan) ---
let gas_bpm = 0;
if (strategi_bpm === 2 && strategi_gas > 0) gas_bpm = 1;
else if (strategi_bpm === 1 && strategi_gas > 0) gas_bpm = 2;
else if (strategi_bpm === 0 && strategi_gas > 0) gas_bpm = 3;
else if (strategi_bpm === 2 && strategi_gas === 0) gas_bpm = 4;

// --- ARAH KOMPAS ---
let arah_kompas = 0;
const k = data.kompas;
if ((k >= 0 && k <= 22) || (k >= 338 && k <= 360)) arah_kompas = 1;
else if (k >= 23 && k <= 67) arah_kompas = 2;
else if (k >= 68 && k <= 112) arah_kompas = 3;
else if (k >= 113 && k <= 157) arah_kompas = 4;
else if (k >= 158 && k <= 202) arah_kompas = 5;
else if (k >= 203 && k <= 247) arah_kompas = 6;
else if (k >= 248 && k <= 293) arah_kompas = 7;
else if (k >= 294 && k <= 337) arah_kompas = 8;

// --- STRATEGI KOMPAS KOMBINASI ---
let strategi_kompas = 0;
if (strategi_bpm === 2 && strategi_shock === 1) strategi_kompas = 4;
else if (strategi_gas === 2 && strategi_shock === 1) strategi_kompas = 3;
else if (strategi_shock === 1 && strategi_bpm === 0) strategi_kompas = 2;
else if (strategi_gas === 2) strategi_kompas = 1;
else if (strategi_bpm === 2) strategi_kompas = 5;

// --- ARAH BAHAYA dan ARAH AMAN ---
if (!context.global.get("arahBerbahaya")) context.global.set("arahBerbahaya", 0);
let arahBerbahaya = context.global.get("arahBerbahaya");
if (strategi_gas === 2) {
  arahBerbahaya = arah_kompas;
  context.global.set("arahBerbahaya", arahBerbahaya);
}

const arahBerlawanan = { 1: 5, 2: 6, 3: 7, 4: 8, 5: 1, 6: 2, 7: 3, 8: 4 };

let arah_aman = 0;
if (strategi_kompas === 2) arah_aman = 9;
else if ((strategi_kompas === 1 || strategi_kompas === 3) && strategi_bpm === 2) arah_aman = 10;
else if (strategi_kompas === 1 || strategi_kompas === 3) arah_aman = arahBerlawanan[arahBerbahaya];

// --- STRATEGI FINAL 1-12 ---
let strategi_final = 0;

if (strategi_bpm === 2 && strategi_gas === 0 && strategi_shock === 0) {
  strategi_final = 1;
} else if (strategi_bpm === 2 && strategi_gas > 0 && strategi_shock === 0) {
  strategi_final = 2;
} else if (strategi_bpm === 2 && strategi_gas === 0 && strategi_shock === 1) {
  strategi_final = 3;
} else if (strategi_bpm === 2 && strategi_gas > 0 && strategi_shock === 1) {
  strategi_final = 4;
} 

else if (strategi_bpm === 1 && strategi_gas === 0 && strategi_shock === 0) {
  strategi_final = 5;
} else if (strategi_bpm === 1 && strategi_gas > 0 && strategi_shock === 0) {
  strategi_final = 6;
} else if (strategi_bpm === 1 && strategi_gas === 0 && strategi_shock === 1) {
  strategi_final = 7;
} else if (strategi_bpm === 1 && strategi_gas > 0 && strategi_shock === 1) {
  strategi_final = 8;
}

else if (strategi_bpm === 0 && strategi_gas === 0 && strategi_shock === 0) {
  strategi_final = 9;
} else if (strategi_bpm === 0 && strategi_gas > 0 && strategi_shock === 0) {
  strategi_final = 10;
} else if (strategi_bpm === 0 && strategi_gas === 0 && strategi_shock === 1) {
  strategi_final = 11;
} else if (strategi_bpm === 0 && strategi_gas > 0 && strategi_shock === 1) {
  strategi_final = 12;
}

// --- OUTPUT ---
data.strategi_bpm = strategi_bpm;
data.strategi_gas = strategi_gas;
data.strategi_shock = strategi_shock;
data.strategi_kompas = strategi_kompas;
data.arah_kompas = arah_kompas;
data.arah_aman = arah_aman;
data.gas_bpm = gas_bpm;
data.strategi_final = strategi_final;

msg.payload = data;
return msg;
