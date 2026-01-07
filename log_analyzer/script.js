const plot = document.getElementById('plot');

document.getElementById('file').addEventListener('change', e => {
    Papa.parse(e.target.files[0], {
        header: true,
        dynamicTyping: true,
        complete: res => {

            const d = res.data;
            const g = (r,k)=>r[k] ?? r[k.toUpperCase()];

            const t   = d.map(r=>g(r,'_time'));
            const map = d.map(r=>g(r,'_map'));
            const rpm = d.map(r=>g(r,'_rpm'));
            const tps = d.map(r=>g(r,'_tps'));
            const afr = d.map(r=>g(r,'_afr'));
            const ign = d.map(r=>g(r,'_ign'));
            const inj = d.map(r=>g(r,'_inj'));
            const clt = d.map(r=>g(r,'_clt'));
            const spd = d.map(r=>g(r,'_spd'));

            const zoomEnd = Math.floor(t.length * 0.15);

            const traces = [
                {x:t,y:map,mode:'lines',line:{color:'#3498db'}},
                {x:t,y:rpm,yaxis:'y2',mode:'lines',line:{color:'#e74c3c'}},

                {x:t,y:tps,yaxis:'y3',mode:'lines',line:{color:'#9b59b6'}},
                {x:t,y:afr,yaxis:'y4',mode:'lines',line:{color:'#2ecc71'}},

                {x:t,y:ign,yaxis:'y5',mode:'lines',line:{color:'#f1c40f'}},
                {x:t,y:inj,yaxis:'y6',mode:'lines',line:{color:'#e67e22'}},

                {x:t,y:clt,yaxis:'y7',mode:'lines',line:{color:'#1abc9c'}},
                {x:t,y:spd,yaxis:'y8',mode:'lines',line:{color:'#ecf0f1'}},

                // ===== CURSOR LINE TRACE (LAST) =====
                {
                    x: [t[0], t[0]],
                    y: [0, 1],
                    xaxis: 'x',
                    yaxis: 'y',
                    mode: 'lines',
                    hoverinfo: 'skip',
                    showlegend: false,
                    line: { color:'#aaa', width:1, dash:'dot' }
                }
            ];

            Plotly.newPlot(plot, traces, {
                height: 950,
                hovermode: false,
                showlegend: false,
                margin: { t: 40, l: 50, r: 60 },

                xaxis: {
                    anchor: 'y7',
                    autorange: false,
                    range: [0, t[zoomEnd]],
                    minallowed: 0,
                    maxallowed: t[t.length - 1],
                    side: 'bottom'
                },

                yaxis :{domain:[0.82,1]},
                yaxis2:{overlaying:'y',side:'right'},

                yaxis3:{domain:[0.54,0.72]},
                yaxis4:{overlaying:'y3',side:'right'},

                yaxis5:{domain:[0.26,0.44]},
                yaxis6:{overlaying:'y5',side:'right'},

                yaxis7:{domain:[0.0,0.18]},
                yaxis8:{overlaying:'y7',side:'right'},

                paper_bgcolor:'#0f1117',
                plot_bgcolor:'#0f1117',
                font:{color:'#eaeaea'}

            }, {
                scrollZoom: true,
                responsive: true
            });

            // ===============================
            // CUSTOM HOVER + CURSOR MOVE
            // ===============================
            const hoverBox = document.getElementById('hoverBox');
            const cursorIndex = traces.length - 1;

            plot.onmousemove = ev => {
                const bb = plot.getBoundingClientRect();
                const x = ev.clientX - bb.left;
                const y = ev.clientY - bb.top;
                if (x < 0 || x > bb.width) return;

                const pct = x / bb.width;
                const i = Math.round(pct * (t.length - 1));
                if (i < 0 || i >= t.length) return;

                hoverBox.style.display = 'block';
                hoverBox.style.left = `${x + 10}px`;
                hoverBox.style.top  = `${y}px`;

                hoverBox.innerHTML = `
                    <span style="color:#3498db">MAP:</span> ${map[i]} kPa<br>
                    <span style="color:#e74c3c">RPM:</span> ${rpm[i]}<br><br>

                    <span style="color:#9b59b6">TPS:</span> ${tps[i]} %<br>
                    <span style="color:#2ecc71">AFR:</span> ${afr[i]}<br><br>

                    <span style="color:#f1c40f">IGN:</span> ${ign[i]}°<br>
                    <span style="color:#e67e22">INJ:</span> ${inj[i]}<br><br>

                    <span style="color:#1abc9c">CLT:</span> ${clt[i]}°C<br>
                    <span style="color:#ecf0f1">SPD:</span> ${spd[i]}
                `;

                // MOVE CURSOR LINE
                Plotly.restyle(plot, {
                    x: [[t[i], t[i]]]
                }, [cursorIndex]);
            };

            plot.onmouseleave = () => {
                hoverBox.style.display = 'none';
            };

            // UI cleanup
            document.querySelector('.top').style.display = 'none';
            document.getElementById('resetBtn').style.display = 'inline-block';
        }
    });
});

// ===============================
// RESET
// ===============================
document.getElementById('resetBtn').onclick = () => {
    Plotly.purge(plot);
    document.getElementById('file').value = '';
    document.querySelector('.top').style.display = 'block';
    document.getElementById('resetBtn').style.display = 'none';
};

// ===============================
// FADE IN
// ===============================
window.addEventListener('load', () => {
    document.body.style.opacity = '1';
});
