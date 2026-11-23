const {webkit,chromium}=require('playwright');
const fs=require('fs');const path=require('path');const assert=require('assert');
const source=fs.readFileSync(path.join(__dirname,'../src/webui.html'),'utf8');
const version=(fs.readFileSync(path.join(__dirname,'../src/Version.h'),'utf8').match(/FW_VERSION "([^"]+)"/)||[])[1];
const html=source.replace('@FW_VERSION@',version);
const fixture={adsbLolKeySet:true,aviationstackKeySet:false,ssid:'Testnetz',hostname:'pixelstop',timeZone:'Europe/Berlin',stopName:'Pohlandplatz',stopId:'fixture',weatherCity:'Dresden',weatherLat:51,weatherLon:13,planePlace:'Dresden',planeLat:51,planeLon:13,nightStart:1320,nightEnd:420,nightEnabled:true,brightness:70,nightBrightness:12,rotationMask:1,rotationSec:[30,20,20,15,20,20,20,20,20],departureCount:4,departurePoll:30,weatherPoll:600,planeRadiusKm:25,planePoll:10,planePlaceToggleSec:10,trackerStatusPollMin:15,vvoMotMask:127,countdowns:[]};
// Pages are reached through the tab bar under the title, like a user would.
async function go(page,id){await page.locator(`nav [data-p="${id}"]`).click();assert(await page.locator(`#${id}`).evaluate(e=>e.classList.contains('on')),'Tab shows its page');}
(async()=>{
 const engine=process.env.SMALLTV_BROWSER==='chromium'?chromium:webkit;
 const browser=await engine.launch({headless:true,...(process.env.SMALLTV_WEBKIT_EXECUTABLE?{executablePath:process.env.SMALLTV_WEBKIT_EXECUTABLE}:{})});
 try {
  for(const width of [320,375,390,430,560,561,768,820,1024]){
   const context=await browser.newContext({viewport:{width,height:1000},isMobile:true,hasTouch:true});
   const page=await context.newPage();
   let posted=null;const config={...fixture};
   await page.route('http://smalltv.test/**',r=>{
    if(r.request().method()==='POST'){posted=r.request().postDataJSON();Object.assign(config,posted);return r.fulfill({contentType:'application/json',body:JSON.stringify({ok:true,reboot:false})});}
    return r.fulfill({contentType:r.request().url().endsWith('/')?'text/html':'application/json',body:r.request().url().endsWith('/')?html:JSON.stringify(r.request().url().includes('/api/config')?config:{ok:true})});
   });
   await page.goto('http://smalltv.test/');
   await page.waitForFunction(()=>document.getElementById('nightStart').value==='22:00');
   assert.strictEqual(await page.locator('#timeZone').inputValue(),'Europe/Berlin');
   // Collapsible cards (v0.5.4.6): only the first card of a tab is open at first;
   // services stay last and closed. Then everything is opened for the checks below.
   assert(await page.locator('#geoAdvanced').evaluate(e=>e.classList.contains('collapsed')),'Advanced settings start closed');
   assert(await page.locator('#serviceInfo').evaluate(e=>e.parentElement.lastElementChild===e&&e.classList.contains('collapsed')));
   assert(!(await page.locator('[data-k="rot"]').evaluate(e=>e.classList.contains('collapsed'))),'First card of a tab starts open');
   assert(await page.locator('[data-k="light"]').evaluate(e=>e.classList.contains('collapsed')));
   await page.evaluate(()=>document.querySelectorAll('.card.fold.collapsed').forEach(c=>c.querySelector('h2').click()));
   assert(await page.evaluate(()=>JSON.parse(localStorage.getItem('pixelstop.fold')).light===true),'Open cards are remembered');
   assert(!(await page.locator('#wetter').textContent()).includes('Ortsauswahl über diesen Browser'));
   assert(!(await page.locator('#flugradar').textContent()).includes('Ortsauswahl über diesen Browser'));

   assert.deepStrictEqual(await page.locator('#modeRows .modeLeft span').allTextContents(),['Abfahrten','Wetter','3-Tage-Vorhersage','Flugradar','Flug-Tracker','Uhr','Countdown 1','Countdown 2','Countdown 3','Countdown 4']);
   assert.strictEqual(await page.locator('#brightness').getAttribute('min'),'1');
   assert.strictEqual(await page.locator('#nightBrightness').getAttribute('min'),'1');
   assert.strictEqual(await page.locator('#timeZone option').count(),50);
   assert.strictEqual(await page.locator('#uhr #timeZone').count(),1,'Time zone lives on the clock tab');
   assert.strictEqual(await page.locator('[data-k="light"] #nightStart').count(),1,'Night mode belongs to brightness');
   const layout=await page.evaluate(()=>{
    const a=document.getElementById('nightStart'),b=document.getElementById('nightEnd');
    const rect=e=>{const r=e.getBoundingClientRect();return {left:r.left,right:r.right,top:r.top,bottom:r.bottom,width:r.width,height:r.height}};
    return {a:rect(a),b:rect(b),card:rect(a.closest('.card')),overflow:document.documentElement.scrollWidth>innerWidth};
   });
   for(const box of [layout.a,layout.b]){assert(box.left>=layout.card.left+15&&box.right<=layout.card.right-15,JSON.stringify({width,layout}));assert(box.height<=46);}
   if(width<=560)assert(layout.b.top>=layout.a.bottom+5);
   else {assert(layout.b.left-layout.a.right>=9);assert(Math.abs(layout.a.width-layout.b.width)<1);}
   assert(!layout.overflow,`Page overflow at ${width}`);
   // Tabs wrap instead of scrolling; every row is centred, at most two rows on a phone.
   const tabs=await page.evaluate(()=>{const nav=document.querySelector('nav').getBoundingClientRect();const rows={};for(const b of document.querySelectorAll('nav .tab')){const r=b.getBoundingClientRect();const k=Math.round(r.top);(rows[k]=rows[k]||[]).push([r.left,r.right])}return {nav:[nav.left,nav.right],rows:Object.values(rows).map(r=>[Math.min(...r.map(x=>x[0])),Math.max(...r.map(x=>x[1]))]),scroll:document.querySelector('nav').scrollWidth>document.querySelector('nav').clientWidth}});
   assert(!tabs.scroll,'Tab bar must not scroll');
   assert(tabs.rows.length<=(width<380?3:2),`Too many tab rows at ${width}: ${tabs.rows.length}`);
   for(const [l,r] of tabs.rows)assert(Math.abs((l-tabs.nav[0])-(tabs.nav[1]-r))<=2,`Tab row not centred at ${width}`);
   assert.deepStrictEqual(await page.locator('#planePoll option').evaluateAll(o=>o.map(x=>x.value)),['10','15','30','60','180']);
   await go(page,'flugradar');
   // Display name: dialog only after a clear move of the location; "Leeren" empties the name.
   await page.locator('#planeDisplayName').fill('ZUHAUSE');
   await page.evaluate(()=>setMapPoint('plane',51.001,13.001));
   assert(await page.locator('#nameDialog').evaluate(e=>e.classList.contains('hidden')),'Small moves keep the name silently');
   await page.evaluate(()=>setMapPoint('plane',41.97,-87.9));
   assert(await page.locator('#nameDialog').isVisible(),'Dialog after a clear move');
   assert.strictEqual(await page.locator('#nameDialogName').textContent(),'„ZUHAUSE“');
   await page.locator('#nameDialog [data-name="clear"]').click();
   assert.strictEqual(await page.locator('#planeDisplayName').inputValue(),'');
   assert(!(await page.locator('#nameDialog').isVisible()));
   await page.locator('#planeDisplayName').fill('ZUHAUSE');
   await page.evaluate(()=>setMapPoint('plane',48.35,11.79));
   assert(!(await page.locator('#nameDialog').isVisible()),'Asked once per location change until saved');
   await page.evaluate(()=>setMapPoint('plane',51,13));
   assert(await page.locator('#planePlaceToggleSec').isDisabled());
   await page.locator('#planeShowPlace').check();assert(await page.locator('#planePlaceToggleSec').isEnabled());
   await page.locator('#planePlaceToggleSec').selectOption('0');
   assert(await page.locator('#planeShowAge').isChecked(),'Age defaults to enabled for older settings');
   await page.locator('#planeShowAge').uncheck();
   const style=await page.evaluate(()=>{
    const h=getComputedStyle(document.querySelector('#abfahrten .subhead'));
    const l=getComputedStyle(document.querySelector('#abfahrten label'));
    return {head:[h.color,h.fontSize,h.fontWeight],label:[l.color,l.fontSize,l.fontWeight]};
   });
   assert.deepStrictEqual(style.head,style.label);
   if(process.env.SMALLTV_SCREENSHOTS&&[390,820].includes(width)){
    await go(page,'anzeige');
    await page.locator('#nightStart').scrollIntoViewIfNeeded();
    await page.screenshot({path:path.join(process.env.SMALLTV_SCREENSHOTS,`night-${width}.png`)});
   }
   await go(page,'uhr');
   await page.locator('#timeZone').selectOption('Asia/Tokyo');
   // Clock: afterglow only for segment styles; all clock settings are saved.
   assert(await page.locator('#clockGhost').isDisabled(),'Afterglow off for the pixel style');
   await page.locator('#clockStyle').selectOption('1');assert(await page.locator('#clockGhost').isEnabled());
   await page.locator('#clock12h').selectOption('1');await page.locator('#clockSeconds').check();
   await page.getByRole('button',{name:'Einstellungen speichern',exact:true}).click();
   await page.waitForFunction(()=>document.querySelector('.toast')?.textContent.includes('gespeichert'));
   assert(posted&&posted.timeZone==='Asia/Tokyo','Selected zone missing in save payload');
   assert(posted.clockStyle===1&&posted.clock12h===true&&posted.clockSeconds===true&&posted.clockDate===true&&/^#[0-9a-f]{6}$/.test(posted.clockColor),'Clock settings missing in save payload');
   assert.strictEqual(posted.countdownGhost,true,'Countdown afterglow missing in save payload');
   assert.strictEqual(posted.planePlaceToggleSec,0,'No-switch option missing in save payload');
   assert.strictEqual(posted.planeShowAge,false,'Disabled age missing in save payload');
   await page.waitForFunction(()=>document.getElementById('timeZone').value==='Asia/Tokyo');
   assert(!(await page.locator('#planeShowAge').isChecked()),'Disabled age restored incorrectly');
   if(width===375){
    // Stop search results from the VVO are text, never markup (audit F-001).
    await page.route('http://smalltv.test/api/vvo/search**',r=>r.fulfill({contentType:'application/json',body:JSON.stringify({stops:[{id:'7',name:'Evil',place:'Dresden',label:'<img src=x onerror=alert(1)> Evil (Dresden)'}]})}));
    await go(page,'abfahrten');await page.locator('#stopName').fill('Evil');await page.locator('#searchStopBtn').click();
    await page.locator('#stopResults .pickres').waitFor();
    assert.strictEqual(await page.locator('#stopResults img').count(),0,'Stop labels must render as text');
    assert((await page.locator('#stopResults .pickres').textContent()).startsWith('<img'),'Label shown literally');
    await page.locator('#stopResults .pickres').click();
    assert.strictEqual(await page.locator('#stopId').inputValue(),'7');
    // Deleting a stored key: button only with a stored key, asks first, sends only the delete flag.
    await go(page,'flugtracker');
    assert(!(await page.locator('#aviationstackKeyClear').isVisible()),'No delete button without a stored key');
    await go(page,'flugradar');
    assert(await page.locator('#adsbLolKeyClear').isVisible(),'Delete button for the stored adsb.lol key');
    await page.locator('#adsbLolKeyClear').click();
    assert(await page.locator('#keyDialog').isVisible());
    assert.strictEqual(await page.locator('#keyDialogName').textContent(),'adsb.lol');
    posted=null;await page.locator('#keyDialog [data-keyd="cancel"]').click();
    assert(!(await page.locator('#keyDialog').isVisible())&&posted===null,'Cancel sends nothing');
    await page.locator('#adsbLolKeyClear').click();await page.locator('#keyDialog [data-keyd="delete"]').click();
    await page.waitForFunction(()=>document.querySelector('.toast')?.textContent.includes('gelöscht'));
    assert.deepStrictEqual(posted,{clearAdsbLolKey:true},'Only the delete flag is sent');
    assert(!(await page.locator('#adsbLolKeyClear').isVisible()),'Button hidden after deleting');
    assert.strictEqual(await page.locator('#adsbLolKey').getAttribute('placeholder'),'API-Key eintragen');
    // Countdown: style, sign and start date are sent with start/save (v0.5.4.5).
    let cdBody=null;await page.route('http://smalltv.test/api/countdown',r=>{cdBody=r.request().postDataJSON();return r.fulfill({contentType:'application/json',body:'{"ok":true}'})});
    await go(page,'countdown');await page.locator('#cdCard0 h2.fold').click();
    assert(await page.locator('#cdStartDate0').isVisible(),'Start date shown for day countdowns');
    assert.strictEqual(await page.locator('#cdStyle0').inputValue(),'0');assert.strictEqual(await page.locator('#cdSign0').inputValue(),'1');
    await page.selectOption('#cdStyle0','1');await page.selectOption('#cdSign0','0');
    await page.fill('#cdDate0','2026-12-24');await page.fill('#cdStartDate0','2026-10-01');
    await page.locator('#cdStart0').click();await page.waitForFunction(()=>true);for(let k=0;k<50&&!cdBody;k++)await page.waitForTimeout(20);
    assert(cdBody&&cdBody.action==='start'&&cdBody.style===1&&cdBody.showSign===false&&cdBody.startDate==='2026-10-01'&&cdBody.targetDate==='2026-12-24','Countdown payload');
    // after the start the page reloads the settings and may fold the card again
    await page.waitForFunction(()=>document.querySelector('.toast')?.textContent.includes('Countdown'));
    if(await page.locator('#cdCard0').evaluate(e=>e.classList.contains('collapsed')))await page.locator('#cdCard0 h2.fold').click();
    await page.selectOption('#cdType0','2');assert(!(await page.locator('#cdStartDate0').isVisible()),'No start date for timers');
    assert(await page.locator('#countdownGhost').isChecked(),'Countdown afterglow is on by default');
    // Start/stop of a countdown must not throw away other unsaved changes (v0.5.4.8).
    await page.locator('#countdownGhost').uncheck();cdBody=null;await page.locator('#cdStart0').click();
    for(let k=0;k<50&&!cdBody;k++)await page.waitForTimeout(20);await page.waitForTimeout(300);
    assert(!(await page.locator('#countdownGhost').isChecked()),'Countdown start must keep unsaved afterglow setting');
    await page.locator('#countdownGhost').check();
    // Forecast of the request quota in the radar tab (v0.5.4.8).
    const rotBefore=await page.evaluate(()=>[...Array(10).keys()].map(i=>[document.getElementById('modeOn'+i).checked,document.getElementById('modeSec'+i).value]));const pollBefore=await page.locator('#planePoll').inputValue();
    await page.route('http://smalltv.test/api/status',r=>r.fulfill({contentType:'application/json',body:JSON.stringify({adsbQuota:{h:[10,135],d:[357,470],w:[357,1900],m:[397,4750]}})}));
    await go(page,'flugradar');await page.evaluate(()=>statusNow());
    await page.waitForFunction(()=>!document.getElementById('fc').classList.contains('hidden'));
    await page.selectOption('#planePoll','10');
    await page.evaluate(()=>{for(let i=0;i<10;i++)document.getElementById('modeOn'+i).checked=(i===2);document.getElementById('modeOn2').dispatchEvent(new Event('change',{bubbles:true}))});
    assert((await page.locator('#fcLine').textContent()).includes('Kontingent'),'Forecast mentions the quota at 10 s');
    assert.strictEqual(await page.locator('#fcTable tr').count(),5,'Table has header plus four periods');
    await page.selectOption('#planePoll','180');
    await page.evaluate(()=>{document.getElementById('modeSec2').value='10';for(let i=0;i<10;i++)document.getElementById('modeOn'+i).checked=(i===2||i===0);document.getElementById('planePoll').dispatchEvent(new Event('change',{bubbles:true}))});
    assert((await page.locator('#fcLine').textContent()).length>10,'Forecast line is filled for a slow interval');
    await page.evaluate(()=>{document.getElementById('modeOn2').checked=false;document.getElementById('modeOn2').dispatchEvent(new Event('change',{bubbles:true}))});
    assert((await page.locator('#fcLine').textContent()).includes('nicht in der Rotation'),'No requests without the radar in the rotation');
    await page.evaluate(b=>b.forEach((v,i)=>{document.getElementById('modeOn'+i).checked=v[0];document.getElementById('modeSec'+i).value=v[1]}),rotBefore);await page.selectOption('#planePoll',pollBefore);
    // Sigma-delta dimming is gone (it stopped Wi-Fi, v0.5.4).
    assert.strictEqual(await page.locator('#backlightMode').count(),0);
   }
   if(width===390){
    await go(page,'wetter');
    const requests=[];let releaseFirst;
    await page.route('http://smalltv.test/api/geo/**',()=>{throw new Error('Geo must not use the ESP8266 HTTP endpoints')});
    await page.route('https://nominatim.openstreetmap.org/reverse**',async r=>{
     const u=new URL(r.request().url());requests.push(u);
     assert.match((await r.request().allHeaders()).referer,/\/pixelstop\//,'Browser request must identify the application');
     if(requests.length===1)await new Promise(resolve=>{releaseFirst=resolve});
     const name=requests.length===1?'OLD':'北京';
     return r.fulfill({headers:{'Access-Control-Allow-Origin':'*'},contentType:'application/json',body:JSON.stringify({ok:true,name})});
    });
    await page.evaluate(()=>{setMapPoint('weather',65.69448,343.30078);reversePlace('weather',65.69448,343.30078)});
    assert(Math.abs(Number(await page.locator('#weatherLon').inputValue())+16.69922)<0.00001);
    assert(await page.locator('#saveBtn').isDisabled(),'Save must be blocked during debounce');
    const beforePost=JSON.stringify(posted);await page.evaluate(()=>save());assert.strictEqual(JSON.stringify(posted),beforePost,'Programmatic save must also respect pending place');
    await page.waitForFunction(()=>geoBusy);await page.waitForTimeout(100);
    assert(await page.locator('#saveBtn').isDisabled(),'Save must be blocked during reverse HTTP');
    assert(requests.length===1&&Number(requests[0].searchParams.get('lon'))>=-180&&Number(requests[0].searchParams.get('lon'))<=180);
    await page.evaluate(()=>{for(let i=0;i<10;i++){setMapPoint('weather',39.9,116.4+i*360);reversePlace('weather',39.9,116.4+i*360)}});
    releaseFirst();
    await page.waitForFunction(()=>document.getElementById('weatherCity').value==='北京');
    assert.strictEqual(requests.length,2,'Rapid picks must coalesce to the last request');
    assert.strictEqual(requests[0].searchParams.get('format'),'jsonv2');
    assert(await page.locator('#saveBtn').isEnabled(),'Save must recover after final name');
    assert.strictEqual(await page.locator('#weatherLon').inputValue(),'116.40000');
    await page.unroute('https://nominatim.openstreetmap.org/reverse**');
    const count=requests.length;await page.evaluate(()=>reversePlace('weather',39.9,116.4));await page.waitForFunction(()=>!geoBusy&&!geoPending.weather);assert.strictEqual(requests.length,count,'Repeat coordinate must use browser cache');
    let releaseObsolete;
    await page.route('https://nominatim.openstreetmap.org/reverse**',async r=>{await new Promise(resolve=>{releaseObsolete=resolve});return r.fulfill({headers:{'Access-Control-Allow-Origin':'*'},contentType:'application/json',body:JSON.stringify({ok:true,name:'OBSOLETE'})})});
    await page.route('https://nominatim.openstreetmap.org/search**',r=>r.fulfill({headers:{'Access-Control-Allow-Origin':'*'},contentType:'application/json',body:JSON.stringify([{name:'Dresden',display_name:'Dresden',lat:'51.05',lon:'13.74'}])}));
    await page.evaluate(()=>{setMapPoint('weather',65,-17);reversePlace('weather',65,-17)});
    await page.waitForFunction(()=>geoBusy);await page.waitForTimeout(100);
    await page.locator('#weatherSearch').fill('Dresden');await page.locator('#weatherSearchBtn').click();
    releaseObsolete();await page.locator('#weatherResults .pickres').click();
    await page.waitForFunction(()=>!geoBusy);
    assert.strictEqual(await page.locator('#weatherCity').inputValue(),'Dresden','Old map reply overwrote selected search result');
    let searches=0;
    await page.unroute('https://nominatim.openstreetmap.org/search**');
    await page.route('https://nominatim.openstreetmap.org/search**',r=>{searches++;assert.strictEqual(new URL(r.request().url()).searchParams.get('accept-language'),'de');return r.fulfill({headers:{'Access-Control-Allow-Origin':'*'},contentType:'application/json',body:JSON.stringify([{namedetails:{name:'Praha','name:de':'Prag'},display_name:'<img src=x onerror=alert(1)>',lat:'50.08',lon:'14.42'}])})});
    await page.locator('#weatherGeoGerman').check();
    await page.locator('#weatherSearch').fill('Prag');await page.locator('#weatherSearchBtn').click();
    await page.locator('#weatherResults .pickres').waitFor();
    assert.strictEqual(await page.locator('#weatherResults img').count(),0,'Provider labels must render as text');
    await page.locator('#weatherResults .pickres').click();assert.strictEqual(await page.locator('#weatherCity').inputValue(),'Prag');
    await page.locator('#weatherSearch').fill('Prag');await page.locator('#weatherSearchBtn').click();await page.waitForFunction(()=>!geoBusy&&!geoPending.weather);
    assert.strictEqual(searches,1,'Repeated text search must use cached response');
    assert(await page.evaluate(()=>localStorage.getItem('smalltv.geoCache').includes('Prag')),'Cache must survive a page reload');
    await page.unroute('https://nominatim.openstreetmap.org/reverse**');
    await page.route('https://nominatim.openstreetmap.org/reverse**',r=>r.fulfill({headers:{'Access-Control-Allow-Origin':'*'},contentType:'application/json',body:JSON.stringify({ok:true,name:''})}));
    await page.evaluate(()=>{setMapPoint('weather',0,-150);reversePlace('weather',0,-150)});
    await page.waitForFunction(()=>document.getElementById('weatherResults').textContent.includes('kein Ortsname verfügbar'));
    assert(await page.locator('#saveBtn').isEnabled(),'Nameless points must remain savable');
    await page.getByRole('button',{name:'Einstellungen speichern',exact:true}).click();
    await page.waitForFunction(()=>document.querySelector('.toast')?.textContent.includes('gespeichert'));
    assert(posted.weatherLat===0&&posted.weatherLon===-150&&posted.weatherCity==='');
    assert(await page.evaluate(()=>Number.isNaN(parseCoord(''))));
    await page.unroute('https://nominatim.openstreetmap.org/reverse**');
    let releaseWeather;
    await page.route('https://nominatim.openstreetmap.org/reverse**',async r=>{
     if(new URL(r.request().url()).searchParams.get('lat')==='10.000000')await new Promise(resolve=>{releaseWeather=resolve});
     return r.fulfill({status:502,headers:{'Access-Control-Allow-Origin':'*'},contentType:'application/json',body:JSON.stringify({ok:false,error:'Fixture error'})});
    });
    await page.evaluate(()=>{reversePlace('weather',10,20);reversePlace('plane',11,21)});
    assert(await page.locator('#saveBtn').isDisabled(),'Both queued map requests must block save');
    await page.waitForFunction(()=>geoBusy);await page.waitForTimeout(100);releaseWeather();
    await page.waitForFunction(()=>!geoBusy&&!geoPending.weather&&!geoPending.plane);
    assert(await page.locator('#saveBtn').isEnabled(),'Reverse errors must release save after both maps finish');
    await page.unroute('https://nominatim.openstreetmap.org/reverse**');
    await context.setOffline(true);
    await page.evaluate(()=>searchPlace('weather'));
    assert.strictEqual(await page.locator('#weatherResults').textContent(),'Internetverbindung erforderlich');
    await page.evaluate(()=>toggleMap('plane'));
    assert.strictEqual(await page.locator('#planeResults').textContent(),'Internetverbindung erforderlich');
    assert(await page.locator('#planeMapWrap').evaluate(e=>e.classList.contains('hidden')));
    await page.evaluate(()=>reversePlace('plane',12,22));
    assert.strictEqual(await page.locator('#planeResults').textContent(),'Internetverbindung erforderlich');
    assert(await page.locator('#saveBtn').isEnabled());
    await context.setOffline(false);
    let limited=0;await page.route('https://nominatim.openstreetmap.org/reverse**',r=>{limited++;return r.fulfill({status:429,headers:{'Access-Control-Allow-Origin':'*','Access-Control-Expose-Headers':'Retry-After','Retry-After':'120'},body:''})});
    await page.evaluate(()=>reversePlace('plane',12,22));await page.waitForFunction(()=>!geoBusy&&!geoPending.plane);
    assert(await page.locator('#planeResults').textContent().then(x=>x.includes('Pause')));
    await page.evaluate(()=>reversePlace('weather',13,23));await page.waitForFunction(()=>!geoBusy&&!geoPending.weather);
    assert.strictEqual(limited,1,'Provider pause must apply to both search/map programs');
    assert(await page.locator('#saveBtn').isEnabled(),'Provider pause must release save');
    await go(page,'system');
    await page.locator('#geoProviderUrl').fill('https://geo.example.test/nominatim/');await page.locator('#geoProviderBtn').click();
    assert.strictEqual(await page.evaluate(()=>localStorage.getItem('smalltv.geoProvider')),'https://geo.example.test/nominatim/','Provider must be replaceable without firmware update');
    // Wi-Fi names are shown as text (no markup) and keep blanks at the end.
    await page.route('http://smalltv.test/api/wifi/scan',r=>r.fulfill({contentType:'application/json',body:JSON.stringify({networks:[{ssid:'<img src=x onerror=alert(1)>',rssi:-50,open:false},{ssid:'Heim ',rssi:-60,open:true}]})}));
    await page.locator('#wifiScanBtn').click();
    await page.locator('#wifiResults .pickres').nth(1).waitFor();
    assert.strictEqual(await page.locator('#wifiResults img').count(),0,'SSIDs must render as text');
    await page.locator('#wifiResults .pickres').nth(1).click();
    assert.strictEqual(await page.locator('#ssid').inputValue(),'Heim ','Trailing blank of an SSID must be kept');
    await page.route('http://smalltv.test/api/log',r=>r.fulfill({contentType:'text/plain',body:'Radar.Position | HTTP 429 | Heap 23000 B'}));
    await page.locator('#logBtn').click();
    await page.waitForFunction(()=>document.getElementById('diagnosticLog').textContent.includes('HTTP 429'));
    await page.locator('#logCopyBtn').click();
    await page.waitForFunction(()=>window.getSelection().toString().includes('HTTP 429'));
    const fonts=await page.evaluate(()=>['logBtn','logCopyBtn','rebootBtn'].map(id=>getComputedStyle(document.getElementById(id)).fontSize));assert.strictEqual(new Set(fonts).size,1);

   }
   console.log(`${engine.name()} ${width}px: time fields, spacing, captions, dependent select and time-zone load/save OK`);
   await context.close();
  }
 } finally {await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1});
