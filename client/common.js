// Общие функции для сайта клиента и панели оператора.

const STATUS_NAMES = {
    new: 'Новый',
    confirmed: 'Подтверждён',
    delivering: 'В пути',
    delivered: 'Доставлен',
    cancelled: 'Отменён',
};

function formatPrice(value) {
    return `${value.toLocaleString('ru-RU')} ₽`;
}

function formatDate(isoDate) {
    const [year, month, day] = isoDate.split('-');
    return `${day}.${month}.${year}`;
}

// Создаёт элемент с текстом. Текст вставляется через textContent,
// поэтому данные из заказа не могут внедрить HTML на страницу.
function createElement(tag, className, text) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (text !== undefined) element.textContent = text;
    return element;
}

// Запрос к API сервера. Возвращает разобранный JSON или выбрасывает Error
// с текстом ошибок, которые прислал сервер.
async function api(path, options = {}) {
    let response;
    try {
        response = await fetch(path, {
            ...options,
            headers: { 'Content-Type': 'application/json', ...(options.headers || {}) },
        });
    } catch {
        throw new Error('Нет связи с сервером. Убедитесь, что water_server запущен, '
            + 'и откройте сайт по адресу http://localhost:8080');
    }

    const data = await response.json().catch(() => null);
    if (!response.ok) {
        const errors = data && Array.isArray(data.errors) ? data.errors : [`Ошибка ${response.status}`];
        const error = new Error(errors.join('\n'));
        error.errors = errors;
        throw error;
    }
    return data;
}

function showAlert(element, messages) {
    element.replaceChildren();
    const list = Array.isArray(messages) ? messages : [messages];
    for (const message of list) {
        element.append(createElement('div', '', message));
    }
    element.hidden = false;
}
