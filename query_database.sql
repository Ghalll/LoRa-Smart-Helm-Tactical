from(bucket: "TES_1")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) =>
    r["_measurement"] == "DataSensor_1" and
    (
      r["_field"] == "bmp" or
      r["_field"] == "gas" or
      r["_field"] == "kompas" or
      r["_field"] == "lat" or
      r["_field"] == "lon" or
      r["_field"] == "mpu_x" or
      r["_field"] == "mpu_y" or
      r["_field"] == "mpu_z" or
      r["_field"] == "shock" or
      r["_field"] == "arah_kompas" or
      r["_field"] == "arah_aman" or
      r["_field"] == "strategi_final"
    )
  )
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
  |> map(fn: (r) => {
    status_bpm = if r.bmp == 0.0 or r.bmp < 50.0 then "BAHAYA"
      else if r.bmp > 100.0 then "WASPADA"
      else "AMAN"

    status_gas = if r.gas >= 300.0 then "BAHAYA"
      else if r.gas >= 100.0 then "WASPADA"
      else "AMAN"

    status_shock = if r.shock >= 1.0 then "BAHAYA" else "AMAN"

    strategi_label = if r.strategi_final == 1.0 then "Sensor Terlepas / Personel Pingsan"
      else if r.strategi_final == 2.0 then "Personel Gugur (Gas Beracun)"
      else if r.strategi_final == 3.0 then "Personel Gugur (Ledakan)"
      else if r.strategi_final == 4.0 then "Personel Gugur (Gas & Ledakan)"
      else if r.strategi_final == 5.0 then "Denyut Jantung Tidak Normal"
      else if r.strategi_final == 6.0 then "Evakuasi Darurat (Medis & Gas)"
      else if r.strategi_final == 7.0 then "Personel Shock atau Panik"
      else if r.strategi_final == 8.0 then "Evakuasi Total (Medis, Gas & Ledakan)"
      else if r.strategi_final == 9.0 then "Personel Siap Operasi"
      else if r.strategi_final == 10.0 then "Evakuasi dengan Masker"
      else if r.strategi_final == 11.0 then "Cari Perlindungan (Ledakan)"
      else if r.strategi_final == 12.0 then "Evakuasi Darurat Lengkap"
      else "TIDAK DIKETAHUI"

    return {
      _time: r._time,
      bmp: r.bmp,
      gas: r.gas,
      kompas: r.kompas,
      lat: r.lat,
      lon: r.lon,
      mpu_x: r.mpu_x,
      mpu_y: r.mpu_y,
      mpu_z: r.mpu_z,
      shock: r.shock,
      arah_kompas: if r.arah_kompas == 0.0 then "AMAN"
        else if r.arah_kompas == 1.0 then "UTARA"
        else if r.arah_kompas == 2.0 then "TIMUR LAUT"
        else if r.arah_kompas == 3.0 then "TIMUR"
        else if r.arah_kompas == 4.0 then "TENGGARA"
        else if r.arah_kompas == 5.0 then "SELATAN"
        else if r.arah_kompas == 6.0 then "BARAT DAYA"
        else if r.arah_kompas == 7.0 then "BARAT"
        else if r.arah_kompas == 8.0 then "BARAT LAUT"
        else "TIDAK DIKETAHUI",

      arah_aman: if r.arah_aman == 0.0 then "AMAN"
        else if r.arah_aman == 1.0 then "UTARA"
        else if r.arah_aman == 2.0 then "TIMUR LAUT"
        else if r.arah_aman == 3.0 then "TIMUR"
        else if r.arah_aman == 4.0 then "TENGGARA"
        else if r.arah_aman == 5.0 then "SELATAN"
        else if r.arah_aman == 6.0 then "BARAT DAYA"
        else if r.arah_aman == 7.0 then "BARAT"
        else if r.arah_aman == 8.0 then "BARAT LAUT"
        else if r.arah_aman == 9.0 then "ADA LEDAKAN"
        else if r.arah_aman == 10.0 then "TIDAK BISA EVAKUASI"
        else "TIDAK DIKETAHUI",

      status_bpm: status_bpm,
      status_gas: status_gas,
      status_shock: status_shock,
      strategi_final_label: strategi_label
    }
  })
  |> keep(columns: [
    "_time",
    "bmp", "gas", "kompas", "lat", "lon",
    "mpu_x", "mpu_y", "mpu_z", "shock",
    "arah_kompas", "arah_aman",
    "status_bpm", "status_gas", "status_shock",
    "strategi_final_label"
  ])
  |> yield(name: "strategi_akhir")
