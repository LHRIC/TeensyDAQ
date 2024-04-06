// repl.repl.ignoreUndefined=true

const { InfluxDB, Point } = require('@influxdata/influxdb-client');

/** Environment variables **/
const url = 'http://localhost:8086'
const token = "XhIcTsxmz1IpLlsA6PAP-DnCGmJJTi3ox8zBEXyoEjS8zfKOCmqoDsO7bWOaNIl6kFPM8o2mX6wwADGT_Zya_A=="
let org = `me`
let bucket = `telem2024vip`

const influxDB = new InfluxDB({ url, token })
const writeApi = influxDB.getWriteApi(org, bucket)

let temperature = 24.0;
let time = 0;

for(let i = 0; i < 24; i++) {
    const point = new Point('temperature')
        // time stamp stuff doesn't work for some reason (maybe use Date() instead of time variable)
        .tag('sensor_id', 'temp001', '2024-04-06T10:00:00')
        // .timestamp(time.toISOString())
        .floatField('value', temperature)
        // .floatField('pressure', pressure);
    console.log(` ${point}`)

    writeApi.writePoint(point);

    temperature += 10;  // Increment temperature by 10

    // time.setMinutes(time.getMinutes() + 1); // Increment time by 1 minute
    // time += 1;


}

// individual points adding below does work however, even with the timestamps as data field

// const point1t = new Point('temperature')
//   .tag('sensor_id', 'TLM01', '2024-04-04 10:00:00')
//   .floatField('value', 24.0)
// console.log(` ${point1t}`)

// const point2t = new Point('temperature')
//   .tag('sensor_id', 'TLM01', '2024-04-05 10:00:00')
//   .floatField('value', 30.0)
// console.log(` ${point2t}`)

// const point1p = new Point('pressure')
//   .tag('sensor_id', 'P01', '2024-04-04 10:00:00')
//   .floatField('value', 1000)
// console.log(` ${point1p}`)

// const point2p = new Point('pressure')
//   .tag('sensor_id', 'P01', '2024-04-05 10:00:00')
//   .floatField('value', 2000)
// console.log(` ${point2p}`)

// writeApi.writePoint(point1t)
// writeApi.writePoint(point2t)
// writeApi.writePoint(point1p)
// writeApi.writePoint(point2p)


/**
 * Flush pending writes and close writeApi.
 **/
writeApi.close().then(() => {
  console.log('WRITE FINISHED')
})

// ------------------grafana details------------------
// grafana login username: admin, password: admin (localhost:3000)
// brew services start/stop grafana
// grafana api token: 2YbjJgNOwIxCerSPtooVHUGwaxZInsqiWZay0ehi4ZZwsqaB4Og29uIWAVRSb9ulDH3SfbmkuOei94qJfIVj-A==
// influxdb token: XhIcTsxmz1IpLlsA6PAP-DnCGmJJTi3ox8zBEXyoEjS8zfKOCmqoDsO7bWOaNIl6kFPM8o2mX6wwADGT_Zya_A==
// influxdb org: me
// influxdb bucket: telem2024vip
// influxdb url: http://localhost:8086
// Basic Auth Details: user = lhrc, password = lhrc