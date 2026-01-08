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

			Plotly.newPlot(plot,[
				{x:t,y:map,line:{color:'#3498db'}},
				{x:t,y:rpm,yaxis:'y2',line:{color:'#e74c3c'}},

				{x:t,y:tps,yaxis:'y3',line:{color:'#9b59b6'}},
				{x:t,y:afr,yaxis:'y4',line:{color:'#2ecc71'}},

				{x:t,y:ign,yaxis:'y5',line:{color:'#f1c40f'}},
				{x:t,y:inj,yaxis:'y6',line:{color:'#e67e22'}},

				{x:t,y:clt,yaxis:'y7',line:{color:'#1abc9c'}},
				{x:t,y:spd,yaxis:'y8',line:{color:'#00d4ff'}}
			],{
				height:950,
				hovermode:false,
				showlegend:false,
				margin:{t:40,l:50,r:60,b:20},

				xaxis:{
					anchor:'y7',
					autorange:false,
					range:[0,t[zoomEnd]],
					rangemode:'tozero',
					minallowed:0,
					maxallowed:t[t.length-1],
					side:'bottom'
				},

				yaxis :{domain:[0.82,1], showline: true, showticklabels:true, ticks:'outside'},
				yaxis2:{overlaying:'y',side:'right', showline: true, showticklabels:true, ticks:'outside'},

				yaxis3:{domain:[0.54,0.72], showline: true, showticklabels:true, ticks:'outside'},
				yaxis4:{overlaying:'y3',side:'right', showline: true, showticklabels:true, ticks:'outside'},

				yaxis5:{domain:[0.26,0.44], showline: true, showticklabels:true, ticks:'outside'},
				yaxis6:{overlaying:'y5',side:'right', showline: true, showticklabels:true, ticks:'outside'},

				yaxis7:{domain:[0.0,0.18], showline: true, showticklabels:true, ticks:'outside'},
				yaxis8:{overlaying:'y7',side:'right', showline: true, showticklabels:true, ticks:'outside'},

				paper_bgcolor:'#0f1117',
				plot_bgcolor:'#0f1117',
				font:{color:'#eaeaea'}
			});
			
			const clampAxes = [
				'yaxis',
				'yaxis2',
				'yaxis3',
				'yaxis4',
				'yaxis7',
				'yaxis8'
			];
			plot.on('plotly_relayout', ev => {
				const update = {};
				clampAxes.forEach(ax => {
					const lo = ev[`${ax}.range[0]`];
					const hi = ev[`${ax}.range[1]`];
					if (lo !== undefined && lo < 0) {
						const span = hi - lo;
						update[`${ax}.range`] = [0, span];
					}
				});
				if (Object.keys(update).length) {
					Plotly.relayout(plot, update);
				}
			});
			
			// ---------- LABEL CLICK TO TOGGLE TRACE ----------
			document.querySelectorAll('.axis-label').forEach(label => {
				const traceIndex = Number(label.dataset.trace);
				let visible = true;

				label.style.cursor = 'pointer';

				label.addEventListener('click', () => {
					visible = !visible;

					Plotly.restyle(plot, {
						visible: visible ? true : 'legendonly'
					}, [traceIndex]);

					// visual feedback
					label.style.opacity = visible ? '1' : '0.35';
					label.classList.toggle('disabled', !visible);
				});
			});

			document.querySelector('.top').style.display = 'none';
			document.getElementById('resetBtn').style.display = 'inline-block';

			const hoverBox = document.getElementById('hoverBox');

			// binary search
			const findIndex = xVal => {
				let lo = 0, hi = t.length - 1;
				while (lo < hi) {
					const mid = (lo + hi) >> 1;
					t[mid] < xVal ? lo = mid + 1 : hi = mid;
				}
				return lo;
			};

			// ---------- HOVER + CURSOR ----------
			let raf = null;
			plot.onmousemove = ev => {
				if (raf) return;
				raf = requestAnimationFrame(() => {
					raf = null;

					const bb = plot.getBoundingClientRect();
					const x = ev.clientX - bb.left;
					const y = ev.clientY - bb.top;
					if (x < 0 || x > bb.width) return;

					const xa = plot._fullLayout.xaxis;
					const pct = x / bb.width;
					const xVal = xa.range[0] + pct * (xa.range[1] - xa.range[0]);
					const i = findIndex(xVal);

					hoverBox.style.display = 'block';
					const boxW = 120;
					const boxH = hoverBox.offsetHeight || 120;
					const pad  = 10;

					// ---------- HORIZONTAL ----------
					const rightSpace = window.innerWidth - ev.clientX;
					if (rightSpace < boxW + pad) {
						// show LEFT of cursor
						hoverBox.style.left = `${x - boxW - pad}px`;
					} else {
						// show RIGHT of cursor
						hoverBox.style.left = `${x + pad}px`;
					}

					// ---------- VERTICAL ----------
					const bottomSpace = window.innerHeight - ev.clientY;
					if (bottomSpace < boxH + pad) {
						// show ABOVE cursor
						hoverBox.style.top = `${y - boxH - pad}px`;
					} else {
						// show BELOW cursor
						hoverBox.style.top = `${y + pad}px`;
					}
					
					const f2 = v => Number.isFinite(v) ? v.toFixed(2) : '-';
					const f0 = v => Number.isFinite(v) ? Math.round(v) : '-';

					hoverBox.innerHTML = `
						<span style="color:#3498db">MAP:</span> ${f2(map[i])} kPa<br>
						<span style="color:#e74c3c">RPM:</span> ${f0(rpm[i])}<br><br>

						<span style="color:#9b59b6">TPS:</span> ${f0(tps[i])} %<br>
						<span style="color:#2ecc71">AFR:</span> ${f2(afr[i])}<br><br>

						<span style="color:#f1c40f">IGN:</span> ${f0(ign[i])} °<br>
						<span style="color:#e67e22">INJ:</span> ${f0(inj[i])} %<br><br>

						<span style="color:#1abc9c">CLT:</span> ${f0(clt[i])} °C<br>
						<span style="color:#00d4ff">SPD:</span> ${f0(spd[i])} km/h
					`;

					Plotly.relayout(plot,{
						shapes:[{
							type:'line',
							x0:xVal,x1:xVal,
							y0:0,y1:1,
							xref:'x',yref:'paper',
							line:{color:'#aaa',width:1,dash:'dot'}
						}]
					});
				});
			};

			plot.onmouseleave = () => {
				hoverBox.style.display = 'none';
				Plotly.relayout(plot,{shapes:[]});
			};

			// ---------- RIGHT CLICK PAN ----------
			let pan = false, startX = 0, startRange = null;

			plot.addEventListener('mousedown', e => {
				if (e.button !== 2) return;
				e.preventDefault();
				pan = true;
				startX = e.clientX;
				startRange = [...plot._fullLayout.xaxis.range];
			});

			window.addEventListener('mousemove', e => {
				if (!pan) return;
				const dx = e.clientX - startX;
				const span = startRange[1] - startRange[0];
				const shift = -dx / plot.clientWidth * span;

				Plotly.relayout(plot,{
					'xaxis.range':[startRange[0]+shift,startRange[1]+shift]
				});
			});

			window.addEventListener('mouseup', ()=>pan=false);
			plot.addEventListener('contextmenu', e=>e.preventDefault());

			// ---------- DOUBLE CLICK RESET ----------
			plot.addEventListener('dblclick', () => {
				const xa = plot._fullLayout.xaxis;
				Plotly.relayout(plot,{
					'xaxis.range':[xa.minallowed, xa.maxallowed]
				});
			});
		}
	});
});

document.getElementById('resetBtn').onclick = () => {
	Plotly.purge(plot);
	document.getElementById('file').value = '';
	document.querySelector('.top').style.display = 'block';
	document.getElementById('resetBtn').style.display = 'none';
};

window.addEventListener('load', () => {
	document.body.style.transition = 'opacity 0.5s ease';
	document.body.style.opacity = '1';
});
