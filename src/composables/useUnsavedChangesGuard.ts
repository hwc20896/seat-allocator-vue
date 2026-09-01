import { onBeforeUnmount, onMounted, ref } from 'vue';

export function useUnsavedChangesGuard() {
  const hasUnsavedChanges = ref(false);

  const markDirty = () => {
    hasUnsavedChanges.value = true;
  };

  const markClean = () => {
    hasUnsavedChanges.value = false;
  };

  const handleBeforeUnload = (event: BeforeUnloadEvent) => {
    if (!hasUnsavedChanges.value) return;
    event.preventDefault();
    event.returnValue = '';
  };

  onMounted(() => {
    window.addEventListener('beforeunload', handleBeforeUnload);
  });

  onBeforeUnmount(() => {
    window.removeEventListener('beforeunload', handleBeforeUnload);
  });

  return { hasUnsavedChanges, markDirty, markClean };
}
