## MT8816 Matrix ##

The MT8816 switch matrix is the component that makes it possible to route audio signals from the SLIC units or the tone generator between specific lines without interfering with the others. This is where the exchange happens.

When a connection is established, A_out from Line 1 must be routed to A_in at Line 2 and vice versa.



<style>
.table_component {
    overflow: auto;
    width: 50%;
}

.table_component table {
    border: 2px solid #ffffffff;
    height: 100%;
    width: 100%;
    table-layout: fixed;
    border-collapse: collapse;
    border-spacing: 1px;
    text-align: center;
}

.table_component caption {
    caption-side: top;
    text-align: center;
}

.table_component th {
    border: 1px solid #dededf;
    background-color: #586874ff;
    color: #000000ff;
    padding: 5px;
    text-align: center;
}

.table_component td {
    border: 1px solid #dededf;
    background-color: #49a4a4ff;
    color: #000000ff;
    padding: 1px;
}

/* Gör sista kolumnen lite bredare */
.table_component th:last-child,
.table_component td:last-child {
    min-width: 100px;
    width: 20px;
    text-align: left;
}

/* Gör första kolumnen grå */
.table_component th:first-child,
.table_component td:first-child {
    background-color: #586874ff;
    width: 30px;
}

/* Sätt bredd för alla kolumner utom första och sista */
/* Här kan du ändra bredden för de "inre" kolumnerna */
.table_component th:not(:first-child):not(:last-child),
.table_component td:not(:first-child):not(:last-child) {
    min-width: 1px; /* justera minbredd här */
    max-width: 10px;
    width: 60px;     /* justera fast bredd här */
}

/* Rotera texten på sista raden 90 grader motsols */
.table_component .rotate-text {
    writing-mode: vertical-rl;
    transform: rotate(180deg);
    white-space: nowrap;
    font-size: 12px;
}

/* Fetstil för första och sista kolumnen och raden */
.table_component th:first-child,
.table_component th:last-child,
.table_component td:first-child,
.table_component td:last-child {
    font-weight: bold;
}

.table_component tr:first-child th,
.table_component tr:last-child td {
    font-weight: bold;
}

/* Gör sista raden högre så att texten blir lika stor som övriga */
.table_component tr:last-child td {
    height: 60px;
    vertical-align: middle;
    font-size: 16px;
}
.table_component tr:last-child .rotate-text {
    font-size: 16px;
}
</style>
<div class="table_component" role="region" tabindex="0">
<table>
    <thead>
        <tr>
            <th><br></th>
            <th>Y0</th>
            <th>Y1</th>
            <th>Y2</th>
            <th>Y3</th>
            <th>Y4</th>
            <th>Y5</th>
            <th>Y6</th>
            <th>Y7</th>
            <th><br></th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td>X0</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td>Aout_0</td>
        </tr>
        <tr>
            <td>X1</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td>Aout_1</td>
        </tr>
        <tr>
            <td>X2</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td>Aout_2</td>
        </tr>
        <tr>
            <td>X3</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td>Aout_3</td>
        </tr>
        <tr>
            <td>X4</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td>Aout_4</td>
        </tr>
        <tr>
            <td>X5</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td>Aout_5</td>
        </tr>
        <tr>
            <td>X6</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td>Aout_6</td>
        </tr>
        <tr>
            <td>X7</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td>Aout_7</td>
        </tr>
        <tr>
            <td>X8</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td><br></td>
        </tr>
        <tr>
            <td>X9</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
        </tr>
        <tr>
            <td>X10</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
        </tr>
        <tr>
            <td>X11</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
        </tr>
        <tr>
            <td>X12</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
        </tr>
        <tr>
            <td>X13</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td>DAC1</td>
        </tr>
        <tr>
            <td>X14</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td>DAC2</td>
        </tr>
        <tr>
            <td>X15</td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td>DAC3</td>
        </tr>
        <tr>
            <td></td>
            <td class="rotate-text">Ain_0</td>
            <td class="rotate-text">Ain_1</td>
            <td class="rotate-text">Ain_2</td>
            <td class="rotate-text">Ain_3</td>
            <td class="rotate-text">Ain_4</td>
            <td class="rotate-text">Ain_5</td>
            <td class="rotate-text">Ain_6</td>
            <td class="rotate-text">Ain_7</td>
            <td></td>
        </tr>
    </tbody>
</table>