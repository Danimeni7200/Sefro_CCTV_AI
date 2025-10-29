import { useEffect, useState } from 'react'
import * as echarts from 'echarts'

export default function Home() {
  const [api, setApi] = useState('http://127.0.0.1:9000')
  const [apiToken, setApiToken] = useState('') // user-provided unique token
  const [authToken, setAuthToken] = useState('') // JWT after login
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')
  const [summary, setSummary] = useState<any>(null)
  const [timeRange, setTimeRange] = useState<'hour'|'day'|'week'|'month'|'year'>('day')

  // load persisted API and auth token
  useEffect(() => {
    const savedApi = localStorage.getItem('cloud_api_url')
    const savedJwt = localStorage.getItem('cloud_auth_jwt')
    if (savedApi) setApi(savedApi)
    if (savedJwt) setAuthToken(savedJwt)
  }, [])

  async function login() {
    try {
      const body = new URLSearchParams({ username, password: `${password}::${apiToken}` })
      const res = await fetch(`${api}/auth/token`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body,
        mode: 'cors'
      })
      if (!res.ok) {
        const text = await res.text()
        throw new Error(`HTTP ${res.status}: ${text}`)
      }
      const data = await res.json()
      const jwt = data.access_token || ''
      setAuthToken(jwt)
      localStorage.setItem('cloud_auth_jwt', jwt)
      localStorage.setItem('cloud_api_url', api)
    } catch (err: any) {
      alert(`Login failed: ${err.message}`)
    }
  }

  async function loadSummary() {
    const res = await fetch(`${api}/analytics/summary`, {
      headers: { Authorization: `Bearer ${authToken}` }
    })
    setSummary(await res.json())
  }

  // Fake data for initial dashboard visuals
  const fakeSummary = {
    total_plates: 1287,
    by_camera: { CAM01: 523, CAM02: 411, CAM03: 353 },
    top_confidence: [
      { plate: '21الف456', confidence: 0.98 },
      { plate: '45د34567', confidence: 0.96 },
      { plate: '12ب12345', confidence: 0.95 }
    ]
  }

  // Login screen if not authenticated
  if (!authToken) {
    return (
      <div className="app-center">
        <div className="card card-hero" style={{ width: 460 }}>
          <div style={{textAlign:'center', marginBottom: 12}}>
            <div style={{fontSize:24, fontWeight:700}}>ورود به سامانه</div>
            <div style={{opacity:.8, fontSize:13}}>برای ادامه، مشخصات کاربری خود را وارد کنید</div>
          </div>
          <div className="section">
            <label>آدرس API</label>
            <input value={api} onChange={e => setApi(e.target.value)} />
          </div>
          <div className="section stack">
            <input placeholder="نام کاربری" value={username} onChange={e => setUsername(e.target.value)} />
            <input placeholder="رمز عبور" type="password" value={password} onChange={e => setPassword(e.target.value)} />
            <input placeholder="توکن یکتا" value={apiToken} onChange={e => setApiToken(e.target.value)} />
            <button onClick={login}>
              ورود
            </button>
          </div>
        </div>
      </div>
    )
  }

  

  // Dashboard after auth
  return (
    <div className="layout">
      <aside className="sidebar">
        <h3>داشبورد تشخیص پلاک</h3>
        <div className="section">
          <label>آدرس API</label>
          <input value={api} onChange={e => setApi(e.target.value)} style={{ width: '100%' }} />
        </div>
        <div className="section card">
          <h4>وضعیت</h4>
          <div>ورود انجام شد</div>
          <button onClick={() => { setAuthToken(''); localStorage.removeItem('cloud_auth_jwt'); }}>خروج</button>
        </div>
      </aside>
      <main className="content">
        <div className="grid grid-3">
          <div className="kpi fade-in">
            <div className="label">کل پلاک‌ها</div>
            <div className="value">{(summary?.total_plates ?? fakeSummary.total_plates).toLocaleString('fa-IR')}</div>
            <div className="badge">امروز +{Math.floor(Math.random()*20)}</div>
          </div>
          <div className="kpi fade-in" style={{animationDelay:'.05s'}}>
            <div className="label">دوربین فعال</div>
            <div className="value">{Object.keys(summary?.by_camera ?? fakeSummary.by_camera).length.toLocaleString('fa-IR')}</div>
            <div className="badge">در حال پایش</div>
          </div>
          <div className="kpi fade-in" style={{animationDelay:'.1s'}}>
            <div className="label">میانگین اطمینان برتر</div>
            <div className="value">{Math.round(((summary?.top_confidence ?? fakeSummary.top_confidence).slice(0,3).reduce((a,c)=>a+(c.confidence||0),0)/3)*100)}%</div>
            <div className="badge">24h</div>
          </div>
        </div>

        <div className="grid" style={{marginTop:16}}>
          <div className="card">
            <div style={{display:'flex', alignItems:'center', justifyContent:'space-between'}}>
              <h3>تعداد پلاک بر اساس دوربین</h3>
              <button className="primary" onClick={loadSummary}>بروزرسانی</button>
            </div>
            <table className="table">
              <thead>
                <tr>
                  <th>دوربین</th>
                  <th>تعداد</th>
                </tr>
              </thead>
              <tbody>
                {Object.entries((summary?.by_camera ?? fakeSummary.by_camera)).map(([cam, cnt]: any) => (
                  <tr key={cam}>
                    <td>{cam}</td>
                    <td>{Number(cnt).toLocaleString('fa-IR')}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          <div className="card">
            <h3 style={{marginTop:0}}>پلاک‌های با اطمینان بالا</h3>
            <table className="table">
              <thead>
                <tr>
                  <th>پلاک</th>
                  <th>اطمینان</th>
                </tr>
              </thead>
              <tbody>
                {(summary?.top_confidence ?? fakeSummary.top_confidence).map((r:any, i:number) => (
                  <tr key={i}>
                    <td>{r.plate}</td>
                    <td>{Math.round((r.confidence||0)*100)}%</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>

        {/* Analytics Dashboard Section */}
        <div className="card" style={{marginTop:16}}>
          <div style={{display:'flex', alignItems:'center', justifyContent:'space-between'}}>
            <h3>تحلیل زمانی پلاک‌ها</h3>
            <TimeTabs onChange={() => setTimeout(()=>{},0)} />
          </div>
          <Chart id="chart-time" title="" data={bucketTime(summary ?? fakeSummary, timeRange)} />
        </div>

        <div className="grid" style={{marginTop:16}}>
          <div className="card">
            <h3>سهم هر دوربین</h3>
            <Pie id="pie-camera" data={Object.entries(summary?.by_camera ?? fakeSummary.by_camera)} />
          </div>
          <div className="card">
            <h3>نرخ اطمینان میانگین (ساختگی)</h3>
            <Chart id="chart-confidence" title="" data={bucketConfidence()} />
          </div>
          <div className="card">
            <h3>پراکندگی تردد (ساختگی)</h3>
            <Chart id="chart-traffic" title="" data={bucketTraffic()} />
          </div>
        </div>

        <div className="card" style={{marginTop:16}}>
          <details>
            <summary>نمایش JSON خام</summary>
            <pre dir="ltr" style={{ background: '#0b1220', padding: 12, borderRadius: 8, overflowX: 'auto', marginTop: 8, maxHeight: 280, overflowY: 'auto' }}>
              {JSON.stringify(summary ?? fakeSummary, null, 2)}
            </pre>
          </details>
        </div>
      </main>
    </div>
  )

  // Helper components and functions

  function TimeTabs({ onChange }:{ onChange:()=>void }){
    return (
      <div className="tabs">
        {(['hour','day','week','month','year'] as const).map(k => (
          <button key={k} className={`tab ${timeRange===k?'active':''}`} onClick={()=>{setTimeRange(k); onChange()}}>
            {k==='hour'?'ساعت':k==='day'?'روز':k==='week'?'هفته':k==='month'?'ماه':'سال'}
          </button>
        ))}
      </div>
    )
  }

  function Chart({ id, title, data }: { id: string, title: string, data: { labels: string[], values: number[] } }) {
    useEffect(() => {
      const el = document.getElementById(id) as HTMLDivElement | null
      if (!el) return
      const chart = echarts.init(el, undefined, { renderer: 'canvas' })
      chart.setOption({
        backgroundColor: 'transparent',
        tooltip: { trigger: 'axis' },
        grid: { left: 24, right: 12, top: 36, bottom: 24 },
        title: { text: title, right: 8, top: 4, textStyle: { color: '#cbd5e1', fontSize: 12 } },
        xAxis: { type: 'category', data: data.labels, axisLine: { lineStyle: { color: '#334155' } }, axisLabel: { color: '#9ca3af' } },
        yAxis: { type: 'value', axisLine: { lineStyle: { color: '#334155' } }, splitLine: { lineStyle: { color: 'rgba(148,163,184,0.1)' } }, axisLabel: { color: '#9ca3af' } },
        series: [{ type: 'bar', data: data.values, itemStyle: { color: '#60a5fa' }, barWidth: '60%' }]
      })
      const onResize = () => chart.resize()
      window.addEventListener('resize', onResize)
      return () => { chart.dispose(); window.removeEventListener('resize', onResize) }
    }, [id, title, JSON.stringify(data)])
    return <div id={id} style={{ width: '100%', height: 240 }} />
  }

  function bucketTime(src: any, granularity: 'hour' | 'day' | 'week' | 'month' | 'year') {
    // Fake time-bucket generator; replace with real API timeseries later
    const labels: string[] = []
    const values: number[] = []
    const count = granularity === 'hour' ? 24 : granularity === 'day' ? 7 : granularity === 'week' ? 8 : granularity === 'month' ? 12 : 5
    for (let i = 0; i < count; i++) {
      labels.push(
        granularity === 'hour' ? `${i}` :
        granularity === 'day' ? `روز ${i+1}` :
        granularity === 'week' ? `هفته ${i+1}` :
        granularity === 'month' ? `ماه ${i+1}` : `سال ${1400 + i}`
      )
      values.push(Math.floor(50 + 50 * Math.abs(Math.sin(i * 1.1))))
    }
    return { labels, values }
  }

  function Pie({ id, data }:{ id:string, data: any[] }){
    useEffect(()=>{
      const el = document.getElementById(id) as HTMLDivElement | null
      if (!el) return
      const chart = echarts.init(el)
      chart.setOption({
        backgroundColor: 'transparent',
        tooltip: { trigger: 'item' },
        series: [{
          type: 'pie', radius: ['40%','70%'],
          itemStyle: { borderColor: '#0b0f17', borderWidth: 2 },
          label: { color: '#cbd5e1' },
          data: data.map(([name, value]) => ({ name, value }))
        }]
      })
      const onResize = () => chart.resize()
      window.addEventListener('resize', onResize)
      return () => { chart.dispose(); window.removeEventListener('resize', onResize) }
    }, [id, JSON.stringify(data)])
    return <div id={id} style={{ width: '100%', height: 240 }} />
  }

  function bucketConfidence(){
    const labels = Array.from({length:12}, (_,i)=>`ماه ${i+1}`)
    const values = labels.map((_,i)=>70 + Math.round(10*Math.sin(i*0.8)))
    return { labels, values }
  }

  function bucketTraffic(){
    const labels = Array.from({length:7}, (_,i)=>`روز ${i+1}`)
    const values = labels.map((_,i)=>50 + Math.round(30*Math.cos(i*0.9)))
    return { labels, values }
  }
}


