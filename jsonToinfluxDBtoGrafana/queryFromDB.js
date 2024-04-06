const { InfluxDB, Point } = require('@influxdata/influxdb-client');

/** Environment variables **/
const url = 'http://localhost:8086'
const token = "XhIcTsxmz1IpLlsA6PAP-DnCGmJJTi3ox8zBEXyoEjS8zfKOCmqoDsO7bWOaNIl6kFPM8o2mX6wwADGT_Zya_A=="
let org = `me`
let bucket = `telem2024vip`

const influxDB = new InfluxDB({ url, token })
const queryApi = influxDB.getQueryApi(org)

const fluxQuery = `from(bucket:"telem2024vip") |> range(start: 0) |> filter(fn: (r) => r._measurement == "temperature")`

// gets pressure data from the database that we manually input in the index.js file (commented out now)
const myQuery = async () => {
    for await (const {values, tableMeta} of queryApi.iterateRows(fluxQuery)) {
      const o = tableMeta.toObject(values)
      console.log(
        `${o._time} ${o._measurement} (${o.sensor_id}): ${o._field}=${o._value}`
      )
    }
  }
  
  /** Execute a query and receive line table metadata and rows. */
  myQuery()
    .then(() => {
        console.log('Query finished')
    })
